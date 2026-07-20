#pragma once

#include <QHash>
#include <QList>
#include <QRect>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QWidget>

class QComboBox;
class QGridLayout;
class QLineEdit;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QResizeEvent;
class QScrollArea;
class QVBoxLayout;
class ToggleRow;
class NumericStepperRow;
class OperatorVectorPreview;

struct DisplayOption final {
    QString id;
    QString displayName;
    QRect geometry;
    qreal devicePixelRatio = 1.0;
};

class DashboardPage final : public QWidget {
    Q_OBJECT
public:
    explicit DashboardPage(QWidget* parent = nullptr);
Q_SIGNALS:
    void navigateRequested(const QString& pageKey);
};

class OperatorsPage final : public QWidget {
    Q_OBJECT
public:
    explicit OperatorsPage(QWidget* parent = nullptr);
Q_SIGNALS:
    // Always emit the stable catalog ID, never a display-name-derived route.
    void operatorSelected(const QString& operatorId);
private:
    struct GridMetrics {
        int columns = 4;
        int spacing = 7;
        QSize tileSize{72, 78};
        QSize iconSize{42, 42};
        int fontSize = 8;
        int radius = 9;
        int padding = 5;
    };

    void setSide(const QString& side);
    void rebuildGrid();
    [[nodiscard]] GridMetrics gridMetricsFor(int width, int height, int itemCount) const;
    void resizeEvent(QResizeEvent* event) override;

    QString m_side = QStringLiteral("attackers");
    QString m_filter = QStringLiteral("all");
    QLineEdit* m_search = nullptr;
    QPushButton* m_attackersButton = nullptr;
    QPushButton* m_defendersButton = nullptr;
    QPushButton* m_filterButton = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_gridContent = nullptr;
    QGridLayout* m_grid = nullptr;
    QLabel* m_countLabel = nullptr;
    int m_currentColumns = 0;
    int m_currentGridHeight = 0;
};

class MoreOptionsPage final : public QWidget {
    Q_OBJECT
public:
    explicit MoreOptionsPage(QWidget* parent = nullptr);

    void setAvailableDisplays(const QList<DisplayOption>& displays);
    void setSelectedRegion(const QRect& region, const QString& displayId);
    void clearSelectedRegion();
    void setSelectionPending();
    void setSelectionError(const QString& message);
    void setRegionSaveResult(bool success, const QString& message);
    void setOverlaySettings(
        bool monitoringEnabled,
        bool showSelectionBorder,
        bool pauseWhenCursorHidden,
        bool lowResourceMode
    );

    [[nodiscard]] QRect selectedRegion() const;
    [[nodiscard]] QString selectedDisplayId() const;
    [[nodiscard]] bool overlayMonitoringEnabled() const;

Q_SIGNALS:
    void backRequested();
    void regionSelectionRequested();
    void regionClearRequested();
    void regionSaveRequested(const QRect& region, const QString& displayId);
    void overlayMonitoringEnabledChanged(bool enabled);
    void showSelectionBorderChanged(bool enabled);
    void pauseWhenForegroundChanged(bool enabled);
    void lowResourceMonitoringChanged(bool enabled);

private:
    void refreshRegionFields();
    void updateRegionPreview();
    void updateRegionState();
    void resizeEvent(QResizeEvent* event) override;

    QList<DisplayOption> m_displays;
    QRect m_region;
    QString m_displayId;
    bool m_hasRegion = false;

    QLabel* m_statusPill = nullptr;
    QLabel* m_statusText = nullptr;
    QLabel* m_preview = nullptr;
    QComboBox* m_displayBox = nullptr;
    QLineEdit* m_xField = nullptr;
    QLineEdit* m_yField = nullptr;
    QLineEdit* m_widthField = nullptr;
    QLineEdit* m_heightField = nullptr;
    QLineEdit* m_displayField = nullptr;
    QPushButton* m_selectButton = nullptr;
    QPushButton* m_clearButton = nullptr;
    QPushButton* m_saveButton = nullptr;
    ToggleRow* m_enableMonitoring = nullptr;
    ToggleRow* m_showBorder = nullptr;
    ToggleRow* m_pauseWhenForeground = nullptr;
    ToggleRow* m_lowResourceMode = nullptr;
};

class SaveFilesPage final : public QWidget {
    Q_OBJECT
public:
    explicit SaveFilesPage(QWidget* parent = nullptr);
Q_SIGNALS:
    void exportRequested();
    void importRequested();
    void settingChanged(const QString& key, const QVariant& value);
};

class ClientSettingsPage final : public QWidget {
    Q_OBJECT
public:
    explicit ClientSettingsPage(QWidget* parent = nullptr);
    void setSavedSettings(const QVariantMap& settings);
Q_SIGNALS:
    void settingChanged(const QString& key, const QVariant& value);
    void exitRequested();

private:
    QHash<QString, ToggleRow*> m_toggles;
    QComboBox* m_refreshRateBox = nullptr;
};

class SettingsPage final : public QWidget {
    Q_OBJECT
public:
    explicit SettingsPage(QWidget* parent = nullptr);
    void setSavedSettings(const QVariantMap& settings);
Q_SIGNALS:
    void settingChanged(const QString& key, const QVariant& value);
    void keybindChanged(const QString& key, const QString& value);
    void resetOperatorsRequested();

private:
    QHash<QString, QComboBox*> m_combos;
};

/**
 * One reusable NEXUS operator-settings page for every non-Recruit operator.
 *
 * The page matches the original operator-detail concept and changes its data
 * in-place when an operator is selected. It is not duplicated 76 times.
 *
 * Import/export is intentionally absent from this page. SaveFilesPage owns the
 * one whole-application configuration file containing every operator record.
 */
class OperatorSettingsPage final : public QWidget {
    Q_OBJECT
public:
    explicit OperatorSettingsPage(QWidget* parent = nullptr);

    [[nodiscard]] bool setOperator(const QString& operatorId);
    [[nodiscard]] QString currentOperatorId() const;
    [[nodiscard]] QVariantMap currentSettings() const;
    [[nodiscard]] QVariantMap settingsFor(const QString& operatorId) const;
    [[nodiscard]] QVariantMap allOperatorSettings() const;

    void setSettingsFor(const QString& operatorId, const QVariantMap& settings);
    void replaceAllOperatorSettings(const QVariantMap& allSettings);
    void resetAllOperatorSettings();

Q_SIGNALS:
    void backRequested();
    void saveRequested(const QString& operatorId, const QVariantMap& settings);
    void resetRequested(const QString& operatorId);
    void settingChanged(
        const QString& operatorId,
        const QString& key,
        const QVariant& value
    );
    void rapidFireSelectionChanged(
        const QString& operatorId,
        int rapidFireValue,
        bool enabled
    );
    void screenRegionPageRequested();

private:
    [[nodiscard]] QVariantMap defaultSettingsFor(const QString& operatorId) const;
    [[nodiscard]] QVariantMap defaultWeaponSettings() const;
    [[nodiscard]] QVariantMap activeWeaponSettings() const;

    void applySettings(const QVariantMap& settings);
    void applyActiveWeaponSettings();
    void persistCurrentDraft();
    void setOperatorField(const QString& key, const QVariant& value);
    void setWeaponField(const QString& key, const QVariant& value);
    void setWeaponSlot(const QString& slot);
    void recordUndoSnapshot();
    void restoreSnapshot(const QVariantMap& snapshot);
    void updateHeader();
    void updateVisualization();
    void updateStatus(const QString& text, bool success = false);
    [[nodiscard]] int normalizedRapidFireValue(const QVariantMap& settings) const;
    void setRapidFireValue(int value, bool persist);
    void cycleRapidFireValue();
    void updateRapidFireStatus();

    QString m_operatorId;
    QString m_weaponSlot = QStringLiteral("primary");
    int m_rapidFireValue = 0;
    QHash<QString, QVariantMap> m_drafts;
    QVariantMap m_internalClipboard;
    QVariantMap m_undoSnapshot;
    bool m_loading = false;

    QLabel* m_iconLabel = nullptr;
    QLabel* m_nameLabel = nullptr;
    QLabel* m_sideLabel = nullptr;
    QLabel* m_assetLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_slopeLabel = nullptr;
    QLabel* m_vectorLabel = nullptr;
    QLabel* m_angleLabel = nullptr;
    QLabel* m_speedLabel = nullptr;
    QLabel* m_rapidFireStatusLabel = nullptr;

    QPushButton* m_backButton = nullptr;
    QPushButton* m_primaryButton = nullptr;
    QPushButton* m_secondaryButton = nullptr;
    QPushButton* m_copyButton = nullptr;
    QPushButton* m_pasteButton = nullptr;
    QPushButton* m_deleteButton = nullptr;
    QPushButton* m_undoButton = nullptr;
    QPushButton* m_resetButton = nullptr;
    QPushButton* m_saveButton = nullptr;

    NumericStepperRow* m_xAmount = nullptr;
    NumericStepperRow* m_yAmount = nullptr;
    NumericStepperRow* m_timeDelay = nullptr;
    OperatorVectorPreview* m_vectorPreview = nullptr;

    ToggleRow* m_profileEnabled = nullptr;
    ToggleRow* m_autoLoad = nullptr;
    ToggleRow* m_showOverlay = nullptr;
    ToggleRow* m_monitorWhileActive = nullptr;
    QPushButton* m_rapidFireButton = nullptr;
    QPlainTextEdit* m_notes = nullptr;
};
