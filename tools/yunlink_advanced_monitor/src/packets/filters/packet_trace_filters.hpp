#ifndef YUNLINK_ADVANCED_MONITOR_PACKETS_PACKET_TRACE_FILTERS_HPP
#define YUNLINK_ADVANCED_MONITOR_PACKETS_PACKET_TRACE_FILTERS_HPP

#include <QString>

class QCheckBox;
class QComboBox;
class QLineEdit;

QString packet_filter_signature(const QComboBox* direction,
                                const QComboBox* transport,
                                const QComboBox* family,
                                const QCheckBox* errors_only,
                                const QLineEdit* search);

#endif  // YUNLINK_ADVANCED_MONITOR_PACKETS_PACKET_TRACE_FILTERS_HPP
