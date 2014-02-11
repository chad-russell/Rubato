#include <QtGui/QGuiApplication>
#include "qtquick2applicationviewer.h"
#include <QtQml>
#include "model.h"
#include "frequencyimageprovider.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    Model* model = new Model();
    QQuickView view;
    view.rootContext()->setContextProperty("model", model);
    view.engine()->addImageProvider(QString("frequency"), model->frequencyImageProvider);
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.setSource(QUrl::fromLocalFile("qml/Rubato/main.qml"));
    view.show();

    return app.exec();
}
