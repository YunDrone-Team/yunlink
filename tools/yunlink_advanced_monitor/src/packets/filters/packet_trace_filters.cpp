#include "packets/filters/packet_trace_filters.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>

QString packet_filter_signature(const QComboBox* direction,
                                const QComboBox* transport,
                                const QComboBox* family,
                                const QCheckBox* errors_only,
                                const QLineEdit* search) {
    return QString("%1|%2|%3|%4|%5")
        .arg(direction != nullptr ? direction->currentIndex() : 0)
        .arg(transport != nullptr ? transport->currentIndex() : 0)
        .arg(family != nullptr ? family->currentIndex() : 0)
        .arg(errors_only != nullptr && errors_only->isChecked() ? 1 : 0)
        .arg(search != nullptr ? search->text().trimmed().toLower() : QString());
}
