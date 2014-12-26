
#include "metadata/metadata.h"
#include "trimwhitespace.h"

namespace Coquillo {
    namespace Filter {
        MetaData::MetaData TrimWhiteSpace::filter(MetaData::MetaData data) const {
            const QRegExp trim("\\s{2,}");
            foreach (const QString field, data.fields()) {
                data.insert(field, data[field].toString().trimmed().remove(trim));
            }
            return data;
        }
    }
}