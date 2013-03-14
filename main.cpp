#include <QDir>
#include <stdio.h>
#include <QString>
#include <QFile>
#include <QRegExp>
#include <QtDebug>

unsigned int totalTestingFunCount;
unsigned int totalVirtualCount;
unsigned int totalDocumentedCount;

QString commentFreeString(QString str)
{
	int docCount = str.count(QString("///"));
	docCount += str.count(QString("/**"));
	totalDocumentedCount += docCount;
	QRegExp reg = QRegExp("/\\*.*\\*/");
	reg.setMinimal(true);
	str.remove(reg);
	str.remove(QRegExp("//[^\\n$]*(\\n|$)"));
	return str;
}

unsigned int virtualMethodsCount(QString str)
{
	return str.count(QRegExp("\\bvirtual\\b"));
}

unsigned int macrosesWithParameters(QString str)
{
	return str.count(QRegExp("Q_[A-Z]+(\\s)*\\([^\\)]*\\)"));
}

unsigned int localFunctionCount(QString path, QString fileName)
{
	QFile file(path + fileName + ".h");
	// the second condition need for events when .h is interface
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text) || !QFile::exists(path + fileName + ".cpp")) {
		return 0;
	}

	QString header = file.readAll();
	header = commentFreeString(header);
	//qDebug() << header;

	QRegExp reg("[^A-Za-z0-9~_]~?[_A-Za-z0-9]+(\\s)*\\([^;]*;");
	unsigned int virtCount = virtualMethodsCount(header);
	unsigned int localCount = header.count(reg) - virtCount - macrosesWithParameters(header);

	totalVirtualCount += virtCount;
	return localCount;
}

void totalFunctionCount(QString dir)
{
	QString filter = "*.h";
	QDir directory(dir);
	QStringList fileList = directory.entryList(QDir::Files);
	QStringList filtFileList = fileList.filter(QRegExp(filter, Qt::CaseInsensitive, QRegExp::Wildcard));
	for (int i = 0; i < filtFileList.length(); ++i)
	{
		QString file = filtFileList.at(i);
		file.chop(2);
		totalTestingFunCount += localFunctionCount(dir, file);
	}
	QStringList dirList = directory.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
	qDebug() << dir << "\ntesting: " << totalTestingFunCount << "\n";
	for (int i = 0; i < dirList.length(); ++i)
	{
		if (dirList.at(i) != "thirdparty") {
			totalFunctionCount(dir + dirList.at(i) + "/");
		}
	}
}

int main(int argc, char *argv[])
{
	totalFunctionCount(QString("D:/QReal/qreal/"));
	qDebug() << "virtual: " << totalVirtualCount << "\n";
	qDebug() << "total: " << totalVirtualCount + totalTestingFunCount << "\n";
	qDebug() << "documented: " << totalDocumentedCount << "\n";
	return 0;
}
