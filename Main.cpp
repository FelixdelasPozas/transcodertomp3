#include <QApplication>
#include "MusicConverter.h"

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	MusicConverter musicConverter;
	musicConverter.show();
	return app.exec();
}


