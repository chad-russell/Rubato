#include <QtGui/QGuiApplication>
#include "qtquick2applicationviewer.h"
#include <QtQml>
#include "model.h"
#include "frequencyimageprovider.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

//    Model* model = new Model();
//    QQuickView view;
//    view.rootContext()->setContextProperty("model", model);
//    view.engine()->addImageProvider(QString("frequency"), model->frequencyImageProvider);
//    view.setResizeMode(QQuickView::SizeRootObjectToView);
//    view.setSource(QUrl::fromLocalFile("qml/Rubato/main.qml"));
//    view.show();

    PhaseVocoder *pv = new PhaseVocoder();
    pv->open_audio_file("/Users/chadrussell/Desktop/input.wav");
    pv->set_timestretch_ratio(2);

    for(int i = 0; i < 40000; i++)
    {
        pv->fft_routine();
        pv->an_idx += 1;
        pv->tempo_an_idx += 1;
    }

    pv->write_output("/Users/chadrussell/Desktop/output.wav");

    app.quit();
}
