#include "mainclass.h"
#include <QDir>
#include <QFile>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include <QProcess>
#include <QChar>
#include <QTime>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp>
#include <xlsxwriter.h>

namespace pt = boost::property_tree;

mainClass::mainClass(QObject *parent) : QObject(parent)
{
    returnCode = 0;
    letterIndex = 65;
}

void mainClass::log(QString message)
{
    QString temp;
    temp = message + "\n";
    printf(temp.toLocal8Bit().data());
}

void mainClass::setParameters(QString host, QString port, QString user, QString pass, QString schema, QString createXML, QString outputFile, bool includeProtected, QString tempDir)
{
    this->host = host;
    this->port = port;
    this->user = user;
    this->pass = pass;
    this->schema = schema;
    this->outputFile = outputFile;
    this->includeSensitive = includeProtected;
    this->tempDir = tempDir;
    this->createXML = createXML;
}

void mainClass::getFieldData(QString table, QString field, QString &desc, QString &valueType, int &size, int &decsize)
{
    for (int pos = 0; pos <= tables.count()-1;pos++)
    {
        if (tables[pos].name == table)
        {
            for (int pos2 = 0; pos2 <= tables[pos].fields.count()-1; pos2++)
            {
                if (tables[pos].fields[pos2].name == field)
                {
                    desc = tables[pos].fields[pos2].desc;
                    valueType = tables[pos].fields[pos2].type;
                    size = tables[pos].fields[pos2].size;
                    decsize = tables[pos].fields[pos2].decSize;
                    return;
                }
            }
        }
    }
    desc = "NONE";
}

const char *mainClass::getSheetDescription(QString name)
{
    QString truncated;
    truncated = name.left(28);
    truncated = truncated.replace("[","");
    truncated = truncated.replace("]","");
    truncated = truncated.replace(":","");
    truncated = truncated.replace("*","");
    truncated = truncated.replace("?","");
    truncated = truncated.replace("/","");
    truncated = truncated.replace("\\","");
    if (tableNames.indexOf(truncated) == -1)
    {
        tableNames.append(truncated);
        return truncated.toUtf8().constData();
    }
    else
    {
        truncated = truncated + "_" + QString::number(letterIndex);
        letterIndex++;
        tableNames.append(truncated);
        return truncated.toUtf8().constData();
    }
}


int mainClass::parseDataToXLSX()
{
    QDir currDir(tempDir);

    lxw_workbook  *workbook  = workbook_new(outputFile.toUtf8().constData());

    //Parse all tables to the Excel file
    for (int pos = 0; pos <= tables.count()-1; pos++)
    {
        if (tables[pos].islookup == false)
        {
            QString sourceFile;
            sourceFile = currDir.absolutePath() + currDir.separator() + tables[pos].name + ".xml";

            pt::ptree tree;
            pt::read_xml(sourceFile.toUtf8().constData(), tree);
            BOOST_FOREACH(boost::property_tree::ptree::value_type const&db, tree.get_child("mysqldump") )
            {
                const boost::property_tree::ptree & aDatabase = db.second; // value (or a subnode)
                BOOST_FOREACH(boost::property_tree::ptree::value_type const&ctable, aDatabase.get_child("") )
                {
                    const std::string & key = ctable.first.data();
                    if (key == "table_data")
                    {
                        const boost::property_tree::ptree & aTable = ctable.second;

                        //Here we need to create the sheet
                        QString tableDesc;
                        tableDesc = tables[pos].desc;
                        if (tableDesc == "")
                            tableDesc = tables[pos].name;
                        lxw_worksheet *worksheet = workbook_add_worksheet(workbook,getSheetDescription(tableDesc));
                        int rowNo = 1;
                        bool inserted = false;
                        BOOST_FOREACH(boost::property_tree::ptree::value_type const&row, aTable.get_child("") )
                        {
                            const boost::property_tree::ptree & aRow = row.second;

                            //Here we need to append a row
                            int colNo = 0;
                            BOOST_FOREACH(boost::property_tree::ptree::value_type const&field, aRow.get_child("") )
                            {
                                const std::string & fkey = field.first.data();
                                if (fkey == "field")
                                {
                                    const boost::property_tree::ptree & aField = field.second;
                                    std::string fname = aField.get<std::string>("<xmlattr>.name");
                                    std::string fvalue = aField.data();
                                    QString desc;
                                    QString valueType;
                                    int size;
                                    int decSize;
                                    QString fieldName = QString::fromStdString(fname);
                                    QString fieldValue = QString::fromStdString(fvalue);
                                    getFieldData(tables[pos].name,fieldName,desc,valueType,size,decSize);
                                    if (desc != "NONE")
                                    {
                                        inserted = true;
                                        if (rowNo == 1)
                                            worksheet_write_string(worksheet,0, colNo, fieldName.toUtf8().constData(), NULL);
                                        worksheet_write_string(worksheet,rowNo, colNo, fieldValue.toUtf8().constData(), NULL);
                                        colNo++;
                                    }
                                }
                            }
                            if (inserted)
                                rowNo++;
                        }
                    }
                }
            }
        }
    }
    //Parse all lookup tables to the Excel file
    for (int pos = 0; pos <= tables.count()-1; pos++)
    {
        if (tables[pos].islookup == true)
        {
            QString sourceFile;
            sourceFile = currDir.absolutePath() + currDir.separator() + tables[pos].name + ".xml";

            pt::ptree tree;
            pt::read_xml(sourceFile.toUtf8().constData(), tree);
            BOOST_FOREACH(boost::property_tree::ptree::value_type const&db, tree.get_child("mysqldump") )
            {
                const boost::property_tree::ptree & aDatabase = db.second; // value (or a subnode)
                BOOST_FOREACH(boost::property_tree::ptree::value_type const&ctable, aDatabase.get_child("") )
                {
                    const std::string & key = ctable.first.data();
                    if (key == "table_data")
                    {
                        const boost::property_tree::ptree & aTable = ctable.second;

                        //Here we need to create the sheet
                        QString tableDesc;
                        tableDesc = tables[pos].desc;
                        if (tableDesc == "")
                            tableDesc = tables[pos].name;
                        lxw_worksheet *worksheet = workbook_add_worksheet(workbook,getSheetDescription(tableDesc));
                        int rowNo = 1;
                        bool inserted = false;
                        BOOST_FOREACH(boost::property_tree::ptree::value_type const&row, aTable.get_child("") )
                        {
                            const boost::property_tree::ptree & aRow = row.second;

                            //Here we need to append a row
                            int colNo = 0;
                            BOOST_FOREACH(boost::property_tree::ptree::value_type const&field, aRow.get_child("") )
                            {
                                const std::string & fkey = field.first.data();
                                if (fkey == "field")
                                {
                                    const boost::property_tree::ptree & aField = field.second;
                                    std::string fname = aField.get<std::string>("<xmlattr>.name");
                                    std::string fvalue = aField.data();
                                    QString desc;
                                    QString valueType;
                                    int size;
                                    int decSize;
                                    QString fieldName = QString::fromStdString(fname);
                                    QString fieldValue = QString::fromStdString(fvalue);
                                    getFieldData(tables[pos].name,fieldName,desc,valueType,size,decSize);
                                    if (desc != "NONE")
                                    {
                                        inserted = true;
                                        if (rowNo == 1)
                                            worksheet_write_string(worksheet,0, colNo, fieldName.toUtf8().constData(), NULL);
                                        worksheet_write_string(worksheet,rowNo, colNo, fieldValue.toUtf8().constData(), NULL);
                                        colNo++;
                                    }
                                }
                            }
                            if (inserted)
                                rowNo++;
                        }
                    }
                }
            }
        }
    }

    workbook_close(workbook);
    return 0;
}

void mainClass::loadTable(QDomNode table)
{
    QDomElement eTable;
    eTable = table.toElement();
    if ((eTable.attribute("sensitive","false") == "false") || (includeSensitive))
    {
        TtableDef aTable;
        aTable.islookup = false;
        aTable.name = eTable.attribute("name","");
        aTable.desc = eTable.attribute("desc","");

        QDomNode field = table.firstChild();
        while (!field.isNull())
        {
            QDomElement eField;
            eField = field.toElement();
            if (eField.tagName() == "field")
            {
                if ((eField.attribute("sensitive","false") == "false") || (includeSensitive))
                {
                    TfieldDef aField;
                    aField.name = eField.attribute("name","");
                    aField.desc = eField.attribute("desc","");
                    aField.type = eField.attribute("type","");
                    aField.size = eField.attribute("size","").toInt();
                    aField.decSize = eField.attribute("decsize","").toInt();
                    aTable.fields.append(aField);
                }
            }
            else
            {
                loadTable(field);
            }
            field = field.nextSibling();
        }
        mainTables.append(aTable);
    }
}

int mainClass::generateXLSX()
{
    QDomDocument docA("input");
    QFile fileA(createXML);
    if (!fileA.open(QIODevice::ReadOnly))
    {
        log("Cannot open input create XML file");
        returnCode = 1;
        return returnCode;
    }
    if (!docA.setContent(&fileA))
    {
        log("Cannot parse input create XML file");
        fileA.close();
        returnCode = 1;
        return returnCode;
    }
    fileA.close();

    QDomElement rootA = docA.documentElement();
    if (rootA.tagName() == "XMLSchemaStructure")
    {
        QDomNode lkpTable = rootA.firstChild().firstChild();

        //Getting the fields to export from Lookup tables
        while (!lkpTable.isNull())
        {
            QDomElement eTable;
            eTable = lkpTable.toElement();
            if ((eTable.attribute("sensitive","false") == "false") || (includeSensitive))
            {
                TtableDef aTable;
                aTable.islookup = true;
                aTable.name = eTable.attribute("name","");
                aTable.desc = eTable.attribute("desc","");

                QDomNode field = lkpTable.firstChild();
                while (!field.isNull())
                {
                    QDomElement eField;
                    eField = field.toElement();
                    if ((eField.attribute("sensitive","false") == "false") || (includeSensitive))
                    {
                        TfieldDef aField;
                        aField.name = eField.attribute("name","");
                        aField.desc = eField.attribute("desc","");
                        aField.type = eField.attribute("type","");
                        aField.size = eField.attribute("size","").toInt();
                        aField.decSize = eField.attribute("decsize","").toInt();
                        aTable.fields.append(aField);
                    }
                    field = field.nextSibling();
                }
                tables.append(aTable);
            }
            lkpTable = lkpTable.nextSibling();
        }

        //Getting the fields to export from tables
        QDomNode table = rootA.firstChild().nextSibling().firstChild();
        loadTable(table);
        for (int nt =mainTables.count()-1; nt >= 0;nt--)
            tables.append(mainTables[nt]);

        //Export the tables as XML to the temp directory
        //Call MySQLDump to export each table as XML
        //We use MySQLDump because it very very fast
        QDir currDir(tempDir);
        QString program = "mysqldump";
        QStringList arguments;
        QProcess *mySQLDumpProcess = new QProcess();        
        QTime procTime;
        procTime.start();
        for (int pos = 0; pos <= tables.count()-1; pos++)
        {            
            arguments.clear();
            arguments << "--single-transaction";
            arguments << "-h" << host;
            arguments << "-u" << user;
            arguments << "--password=" + pass;
            arguments << "--skip-triggers";
            arguments << "--xml";
            arguments << "--no-create-info";
            arguments << schema;
            arguments << tables[pos].name;
            mySQLDumpProcess->setStandardOutputFile(currDir.absolutePath() + currDir.separator() + tables[pos].name + ".xml");
            mySQLDumpProcess->start(program, arguments);
            mySQLDumpProcess->waitForFinished(-1);
            if ((mySQLDumpProcess->exitCode() > 0) || (mySQLDumpProcess->error() == QProcess::FailedToStart))
            {
                if (mySQLDumpProcess->error() == QProcess::FailedToStart)
                {
                    log("Error: Command " +  program + " not found");
                }
                else
                {
                    log("Running mysqldump returned error");
                    QString serror = mySQLDumpProcess->readAllStandardError();
                    log(serror);
                    log("Running paremeters:" + arguments.join(" "));
                }
                return 1;
            }
        }
        delete mySQLDumpProcess;
        returnCode = parseDataToXLSX();

        int Hours;
        int Minutes;
        int Seconds;
        int Milliseconds;

        if (returnCode == 0)
        {
            Milliseconds = procTime.elapsed();
            Hours = Milliseconds / (1000*60*60);
            Minutes = (Milliseconds % (1000*60*60)) / (1000*60);
            Seconds = ((Milliseconds % (1000*60*60)) % (1000*60)) / 1000;
            log("Finished in " + QString::number(Hours) + " Hours," + QString::number(Minutes) + " Minutes and " + QString::number(Seconds) + " Seconds.");
        }
        return returnCode;
    }
    else
    {
        log("The input create XML file is not valid");
        returnCode = 1;
        return returnCode;
    }
}

void mainClass::run()
{
    if (QFile::exists(createXML))
    {
        QDir tDir;
        if (!tDir.exists(tempDir))
        {
            if (!tDir.mkdir(tempDir))
            {
                log("Cannot create temporary directory");
                returnCode = 1;
                emit finished();
            }
        }

        if (generateXLSX() == 0)
        {
            returnCode = 0;
            emit finished();
        }
        else
        {
            returnCode = 1;
            emit finished();
        }
    }
    else
    {
        log("The create XML file does not exists");
        returnCode = 1;
        emit finished();
    }
}
