#include <QCoreApplication>
#include "filehelper.h"
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // grab the filename from the command line arguments
    QString sFilenameIn = argv[1];
    QString sFilenameOut = sFilenameIn + ".fixed";

    qDebug() << "Reordering" << sFilenameIn;
    QStringList slAllLinesIn, slAllLinesOrdered, slAllLinesFinal;

    if(!FileHelper::readAll(sFilenameIn, slAllLinesIn)) {
        qDebug() << "Failed to parse the file";
        return -1;
    }

    if(!FileHelper::reorderGCode(slAllLinesIn, slAllLinesOrdered))
    {
        qDebug() << "Failed to reorder GCode";
        return -1;
    }

    if(!FileHelper::removeDuplicates(slAllLinesOrdered, slAllLinesFinal))
    {
        qDebug() << "Failed to remove duplicate tool changes";
        return -1;
    }

    if(!FileHelper::writeAll(sFilenameOut, slAllLinesFinal)) {
        qDebug() << "Failed to write out fixed file";
        return -1;
    }

    qDebug() << "Successfully reordered" << sFilenameOut;

    return a.exec();
}
