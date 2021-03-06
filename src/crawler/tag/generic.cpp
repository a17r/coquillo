
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>
#include <QImage>
#include <QVariantMap>
#include "generic.hpp"

#include <QBuffer>
#include <QImageWriter>

#include <QDebug>

#define Q2TString(str) TagLib::String((str).toUtf8().data(), TagLib::String::UTF8)
#define T2QString(str) QString::fromUtf8((str).toCString(true))

namespace Coquillo {
    namespace Crawler {
        namespace Tag {
            quint16 Generic::imageId(const QImage & image) {
                const auto bits = reinterpret_cast<const char*>(image.bits());
                return qChecksum(bits, image.byteCount());
            }

            QByteArray Generic::imageToBytes(const QImage & image, const QString & format) {
                QByteArray data;
                QBuffer buffer(&data);
                buffer.open(QIODevice::WriteOnly);
                QImageWriter writer(&buffer, format.split('/').last().toLocal8Bit());
                writer.write(image);

                return data;
            }

            QVariantMap Generic::read(const TagLib::Tag * tag) const {
                return {
                    {"album", T2QString(tag->album())},
                    {"artist", T2QString(tag->artist())},
                    {"comment", T2QString(tag->comment())},
                    {"genre", T2QString(tag->genre())},
                    {"title", T2QString(tag->title())},
                    {"number", tag->track()},
                    {"year", tag->year()},
                };
            }

            void Generic::write(TagLib::Tag * tag, const QVariantMap & values) {
                TagLib::PropertyMap properties;

                for (auto i = values.constBegin(); i != values.constEnd(); i++) {
                    properties[Q2TString(i.key())] = {Q2TString(i.value().toString())};
                }

                tag->setProperties(properties);

                if (values.contains("year")) {
                    tag->setYear(values["year"].toInt());
                }

                if (values.contains("number")) {
                    tag->setTrack(values["number"].toInt());
                }
            }
        }
    }
}
