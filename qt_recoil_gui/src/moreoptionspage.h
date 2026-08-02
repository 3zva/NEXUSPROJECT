#pragma once

#include <QPixmap>
#include <QRect>
#include <QString>
#include <QVariant>
#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class ToggleRow;

/**
 * Persistent NEXUS "More Options" page opened by the top-right ellipsis.
 *
 * This class owns only the configuration UI and its typed integration
 * contract. It intentionally does not implement screen capture, OCR,
 * monitoring loops, global hooks, or the drag-selection overlay itself.
 */
class MoreOptionsPage final : public QWidget {
    Q_OBJECT

public:
    explicit MoreOptionsPage(QWidget* parent = nullptr);

    void setSelectedRegion(
        const QRect& region,
        const QString& displayId,
        const QPixmap& preview = QPixmap()
    );
    void clearSelectedRegion();
    void setRegionSaveResult(bool success, const QString& message = QString());

    [[nodiscard]] QRect selectedRegion() const;
    [[nodiscard]] QString selectedDisplayId() const;
    [[nodiscard]] bool hasValidRegion() const;

Q_SIGNALS:
    void backRequested();
    void regionSelectionRequested();
    void regionClearRequested();
    void regionSaveRequested(const QRect& region, const QString& displayId);
    void overlayMonitoringEnabledChanged(bool enabled);
    void settingChanged(const QString& key, const QVariant& value);

private:
    void refreshDisplays();
    void updateRegionControls();
    void updatePreview(const QPixmap& preview = QPixmap());
    void updateStatus(const QString& text, bool success = false);

    QRect m_region;
    QString m_displayId;

    QComboBox* m_displayCombo = nullptr;
    QLabel* m_previewLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLineEdit* m_xValue = nullptr;
    QLineEdit* m_yValue = nullptr;
    QLineEdit* m_widthValue = nullptr;
    QLineEdit* m_heightValue = nullptr;
    QPushButton* m_clearButton = nullptr;
    QPushButton* m_saveButton = nullptr;
    ToggleRow* m_monitoringToggle = nullptr;
};
