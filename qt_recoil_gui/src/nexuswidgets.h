#pragma once

#include <QCheckBox>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QString>
#include <QToolButton>
#include <QWidget>

class QLineEdit;
class QPaintEvent;

class CardFrame : public QFrame {
    Q_OBJECT
public:
    explicit CardFrame(QWidget* parent = nullptr, bool elevated = false);
};

class PageHeading final : public QWidget {
    Q_OBJECT
public:
    explicit PageHeading(
        const QString& title,
        const QString& subtitle,
        QWidget* parent = nullptr
    );
};

class SidebarButton final : public QPushButton {
    Q_OBJECT
public:
    explicit SidebarButton(
        const QString& text,
        const QString& iconName,
        QWidget* parent = nullptr
    );
    void setActive(bool active);
};

class ToggleRow final : public CardFrame {
    Q_OBJECT
public:
    explicit ToggleRow(
        const QString& title,
        const QString& description,
        bool checked,
        QWidget* parent = nullptr
    );
    [[nodiscard]] bool isChecked() const;
    void setChecked(bool checked);

Q_SIGNALS:
    void toggled(bool checked);

private:
    QCheckBox* m_toggle = nullptr;
};

/**
 * NEXUS numeric control matching the original operator-detail reference:
 * label | minus button | visible value box | plus button | reset button.
 *
 * QDoubleSpinBox arrows are intentionally not used because the design calls
 * for explicit, always-visible + and - controls.
 */
class NumericStepperRow final : public CardFrame {
    Q_OBJECT
public:
    explicit NumericStepperRow(
        const QString& title,
        double minimum,
        double maximum,
        double step,
        int decimals,
        double defaultValue,
        const QString& suffix = QString(),
        QWidget* parent = nullptr
    );

    [[nodiscard]] double value() const;
    [[nodiscard]] double defaultValue() const;
    void setValue(double value, bool emitSignal = false);
    void resetValue();

Q_SIGNALS:
    void valueChanged(double value);

private:
    void changeBy(double delta);
    void updateDisplay();
    [[nodiscard]] double bounded(double value) const;

    double m_minimum = 0.0;
    double m_maximum = 0.0;
    double m_step = 1.0;
    int m_decimals = 0;
    double m_defaultValue = 0.0;
    double m_value = 0.0;
    QString m_suffix;
    QLineEdit* m_valueBox = nullptr;
};

/**
 * Lightweight visualization for the currently displayed X/Y/time values.
 * This is UI-only and does not implement any input automation.
 */
class OperatorVectorPreview final : public QWidget {
    Q_OBJECT
public:
    explicit OperatorVectorPreview(QWidget* parent = nullptr);
    void setValues(double xAmount, double yAmount, double timeDelay);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    double m_xAmount = 0.0;
    double m_yAmount = 0.0;
    double m_timeDelay = 0.001;
};

class ActionCard final : public CardFrame {
    Q_OBJECT
public:
    explicit ActionCard(
        const QString& iconName,
        const QString& title,
        const QString& description,
        QWidget* parent = nullptr
    );

Q_SIGNALS:
    void activated();
};

class OperatorTile final : public QToolButton {
    Q_OBJECT
public:
    explicit OperatorTile(
        const QString& name,
        const QString& iconResource,
        QWidget* parent = nullptr
    );
    [[nodiscard]] QString operatorName() const;
    [[nodiscard]] QString iconResource() const;
    void setDisplayMetrics(const QSize& iconSize, const QSize& minimumSize, int fontSize, int radius, int verticalPadding);

private:
    QString m_name;
    QString m_iconResource;
};

QScrollArea* createPageScrollArea(QWidget* content, QWidget* parent = nullptr);
QLabel* createSectionLabel(const QString& text, QWidget* parent = nullptr);
QPushButton* createAccentButton(const QString& text, QWidget* parent = nullptr);
