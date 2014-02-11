#include "frequencyimageprovider.h"
#include <QDebug>

FrequencyImageProvider::FrequencyImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage FrequencyImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    qDebug() << "received image request for size " << *size;
    if (size)
        *size = image.size();
    return image;
}
