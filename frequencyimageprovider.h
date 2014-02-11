#ifndef FREQUENCYIMAGEPROVIDER_H
#define FREQUENCYIMAGEPROVIDER_H

#include <QQuickImageProvider>

class FrequencyImageProvider : public QQuickImageProvider
{
public:
    FrequencyImageProvider();
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize);
    QImage image;
};

#endif // FREQUENCYIMAGEPROVIDER_H
