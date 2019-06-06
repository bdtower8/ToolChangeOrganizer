#ifndef FILEHELPER_H
#define FILEHELPER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDebug>

#define NUM_EXTRUDERS 4

class FileHelper {

public:
    // read all the lines in a file
    static bool readAll(const QString& sFilename, QStringList& slLines)
    {
        // make sure the file exists
        if(!QFile::exists(sFilename))
        {
            qDebug() << "File" << sFilename << "does not exist";
            return false;
        }

        // read the configuration file
        QFile textFile(sFilename);
        if(!textFile.open(QIODevice::ReadOnly))
        {
            qDebug() << "Cannot open file" << sFilename;
            return false;
        }

        // read all the lines in the file
        QTextStream textStream(&textFile);
        QString sLine = textStream.readLine();
        while(!sLine.isNull())
        {
            slLines.push_back(sLine);
            sLine = textStream.readLine();
        }

        textFile.close();
        return true;
    }

    // write all the strings in the list to the file
    static bool writeAll(const QString &sFilename, const QStringList &slLines)
    {
        QFile file(sFilename);
        QTextStream textStream(&file);

        if(file.open(QFile::WriteOnly | QFile::Truncate))
        {
            for(int i = 0;i < slLines.size();++i)
            {
                textStream << slLines.at(i) << "\n";
            }
        }
        else
        {
            qDebug() << "Unable to open" << sFilename << "for writing";
            return false;
        }

        file.close();
        return true;
    }

    // reorder the gcode to prevent tool swapping mid-layer
    static bool reorderGCode(const QStringList& slAllLinesIn, QStringList& slAllLinesOut)
    {
        QStringList slAllLinesUnprocessed = slAllLinesIn;

        qDebug() << "Parsing" << slAllLinesUnprocessed.length() << "lines";

        // handle everything before layer 1
        while(!slAllLinesUnprocessed.empty()) {
            QString sThisLine = slAllLinesUnprocessed.at(0);

            slAllLinesOut.push_back(sThisLine);
            slAllLinesUnprocessed.pop_front();

            // stop when we reach layer 1
            if(sThisLine.startsWith("; layer 1"))
            {
                // its possible that we want to preserve the T-1 without worrying about it later
                if(slAllLinesUnprocessed.at(0) == "T-1") {
                    slAllLinesOut.push_back(slAllLinesUnprocessed.at(0));
                    slAllLinesUnprocessed.pop_front();
                }

                break;
            }
        }


        int iLayer = 1;
        int iChangesThisLayer = 0;
        int iTotalChanges = 0;
        int iCurrentTool = 0;

        QVector<QStringList> vslPiecesForLayer(NUM_EXTRUDERS);

        bool bPingPongDirection = false;

        // the main loop
        QString sCurrentLine = slAllLinesUnprocessed.at(0);
        while(sCurrentLine != "; layer end")
        {
            // track if we have processed the line or not
            bool bHasBeenHandled = false;

            // if we are at a new layer, handle the previous layer
            if(sCurrentLine.startsWith("; layer") && !bHasBeenHandled)
            {
                qDebug() << "Layer:\t" << iLayer << "Changes:\t" << iChangesThisLayer;

                // swap directions through tools one each pass to double up on back-to-back layers
                if(bPingPongDirection) {
                    // go through all of the tools in order
                    for(int i = 0;i < vslPiecesForLayer.size();++i) {
                        if(vslPiecesForLayer[i].length() > 0) {
                            vslPiecesForLayer[i].push_front("T" + QString::number(i));
                        }

                        slAllLinesOut << vslPiecesForLayer.at(i);
                        vslPiecesForLayer[i].clear();
                    }
                } else {// go through all of the tools in order
                    for(int i = vslPiecesForLayer.size() - 1;i >= 0;--i) {
                        if(vslPiecesForLayer[i].length() > 0) {
                            vslPiecesForLayer[i].push_front("T" + QString::number(i));
                        }

                        slAllLinesOut << vslPiecesForLayer.at(i);
                        vslPiecesForLayer[i].clear();
                    }
                }

                // grab the layer line
                slAllLinesOut.push_back(sCurrentLine);

                // flip directions for next time
                bPingPongDirection = !bPingPongDirection;

                iLayer++;
                iChangesThisLayer = 0;
                bHasBeenHandled = true;
            }

            // if we see a tool change, capture that separately
            if(sCurrentLine.startsWith("T") && !bHasBeenHandled)
            {
                iCurrentTool = sCurrentLine.mid(1).toInt();
                iChangesThisLayer++;
                iTotalChanges++;
                bHasBeenHandled = true;
            }

            if(!bHasBeenHandled) {
                // stick the line on the current tool
                vslPiecesForLayer[iCurrentTool].push_back(sCurrentLine);
            }

            // grab the next line
            slAllLinesUnprocessed.pop_front();
            sCurrentLine = slAllLinesUnprocessed.at(0);
        }

        // capture eveything at the end
        slAllLinesOut << slAllLinesUnprocessed;

        qDebug() << "Total changes:" << iTotalChanges;

        return true;
    }

    // remove tool changes call if the tool doesnt change (makes tracking change delta easier)
    static bool removeDuplicates(const QStringList& slAllLinesIn, QStringList& slAllLinesOut)
    {
        QStringList slAllLinesUnprocessed = slAllLinesIn;

        qDebug() << "Pruning" << slAllLinesUnprocessed.length() << "lines";

        // handle everything before layer 1
        while(!slAllLinesUnprocessed.empty()) {
            QString sThisLine = slAllLinesUnprocessed.at(0);

            slAllLinesOut.push_back(sThisLine);
            slAllLinesUnprocessed.pop_front();

            // stop when we reach layer 1
            if(sThisLine.startsWith("; layer 1"))
            {
                // its possible that we want to preserve the T-1 without worrying about it later
                if(slAllLinesUnprocessed.at(0) == "T-1") {
                    slAllLinesOut.push_back(slAllLinesUnprocessed.at(0));
                    slAllLinesUnprocessed.pop_front();
                }

                break;
            }
        }

        // the main loop
        QString sCurrentLine = slAllLinesUnprocessed.at(0);
        QString sLastChange = "";

        while(sCurrentLine != "; layer end")
        {
            if(sCurrentLine.startsWith("T"))
            {
                if(sCurrentLine != sLastChange) {
                    slAllLinesOut.push_back(sCurrentLine);
                }

                sLastChange = sCurrentLine;
            } else {
                slAllLinesOut.push_back(sCurrentLine);
            }

            // grab the next line
            slAllLinesUnprocessed.pop_front();
            sCurrentLine = slAllLinesUnprocessed.at(0);
        }

        // capture eveything at the end
        slAllLinesOut << slAllLinesUnprocessed;

        return true;
    }
};

#endif // FILEHELPER_H
