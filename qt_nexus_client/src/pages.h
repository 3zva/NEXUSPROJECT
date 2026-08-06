#pragma once

#include <QHash>
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
class QVBoxLayout;
class ToggleRow;
class NumericStepperRow;
class OperatorVectorPreview;

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
    void setSide(const QString& side);
    void rebuildGrid();
    QString m_side = QStringLiteral("attackers");
    QLineEdit* m_search = nullptr;
    QPushButton* m_attackersButton = nullptr;
    QPushButton* m_defendersButton = nullptr;
    QWidget* m_gridContent = nullptr;
    QGridLayout* m_grid = nullptr;
    QLabel* m_countLabel = nullptr;
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
Q_SIGNALS:
    void settingChanged(const QString& key, const QVariant& value);
    void exitRequested();
};

class NativeDetectorPage final : public QWidget {
    Q_OBJECT
public:
    explicit NativeDetectorPage(QWidget* parent = nullptr);
Q_SIGNALS:
    void settingChanged(const QString& key, const QVariant& value);
};

class SettingsPage final : public QWidget {
    Q_OBJECT
public:
    explicit SettingsPage(QWidget* parent = nullptr);
    void setSettings(const QVariantMap& settings);
Q_SIGNALS:
    void settingChanged(const QString& key, const QVariant& value);
    void keybindChanged(const QString& key, const QString& value);
    void resetOperatorsRequested();
private:
    ToggleRow* m_autoUpdates = nullptr;
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
    // Typed GUI-to-backend contract. rapidFireValue uses the schema-v2 mapping:
    // 0 = off, 1 = Weapon 1 / primary, 2 = Weapon 2 / secondary, 3 = both.
    void rapidFireSelectionChanged(
        const QString& operatorId,
        int rapidFireValue,
        bool enabled
    );
    void screenRegionPageRequested();
    // One reusable page emits the selected per-operator weapon and attachment
    // state. converterInputs are resolved by AuthenticatedRoot from the shared
    // Sensitivity & FOV Converter page.
    void loadoutSelectionChanged(
        const QString& operatorId,
        const QString& weaponSlot,
        const QString& selectedWeapon,
        const QVariantMap& attachments
    );

private:
    [[nodiscard]] QVariantMap defaultSettingsFor(const QString& operatorId) const;
    [[nodiscard]] QVariantMap defaultWeaponSettingsForSlot(const QString& slot) const;
    [[nodiscard]] QVariantMap defaultWeaponSettingsForSlot(
        const QString& operatorId,
        const QString& slot
    ) const;
    [[nodiscard]] QVariantMap activeWeaponSettings() const;

    void applySettings(const QVariantMap& settings);
    void applyActiveWeaponSettings();
    void persistCurrentDraft();
    void setOperatorField(const QString& key, const QVariant& value);
    void setWeaponField(const QString& key, const QVariant& value);
    void setAttachmentField(const QString& key, const QVariant& value);
    void populateWeaponSelector();
    void populateAttachmentSelectors();
    void applyLoadoutSelectors(const QVariantMap& weaponSettings);
    [[nodiscard]] QVariantMap activeAttachmentSettings() const;
    void updateAdsBindingLabel();
    void emitCurrentLoadoutSelection();
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
    QLabel* m_pullLabel = nullptr;
    QLabel* m_rampStartLabel = nullptr;
    QLabel* m_rapidFireStatusLabel = nullptr;
    QLabel* m_adsBindingLabel = nullptr;

    QPushButton* m_backButton = nullptr;
    QPushButton* m_primaryButton = nullptr;
    QPushButton* m_secondaryButton = nullptr;
    QPushButton* m_copyButton = nullptr;
    QPushButton* m_pasteButton = nullptr;
    QPushButton* m_deleteButton = nullptr;
    QPushButton* m_undoButton = nullptr;
    QPushButton* m_resetButton = nullptr;
    QPushButton* m_saveButton = nullptr;
    QPushButton* m_rapidFireButton = nullptr;

    QComboBox* m_weaponSelector = nullptr;
    QComboBox* m_opticSelector = nullptr;
    QComboBox* m_barrelSelector = nullptr;
    QComboBox* m_gripSelector = nullptr;
    QComboBox* m_underbarrelSelector = nullptr;

    NumericStepperRow* m_xAmount = nullptr;
    NumericStepperRow* m_yAmount = nullptr;
    NumericStepperRow* m_horizontalRamp = nullptr;
    NumericStepperRow* m_verticalRamp = nullptr;
    NumericStepperRow* m_rampStartSeconds = nullptr;
    NumericStepperRow* m_timeDelay = nullptr;
    OperatorVectorPreview* m_vectorPreview = nullptr;

    ToggleRow* m_profileEnabled = nullptr;
    ToggleRow* m_autoLoad = nullptr;
    ToggleRow* m_showRuntimeHelper = nullptr;
    ToggleRow* m_monitorWhileActive = nullptr;
    QPlainTextEdit* m_notes = nullptr;
};
