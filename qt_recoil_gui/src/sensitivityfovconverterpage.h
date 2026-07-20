#pragma once

#include <QVariantMap>
#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSlider;

/**
 * Native NEXUS Sensitivity & FOV Converter frontend.
 *
 * This page owns presentation, validation, and routing only. It does not
 * invent or duplicate the application's conversion algorithm. Clicking
 * Calculate emits a normalized QVariantMap for the existing converter
 * backend. The backend returns results through setScaleFactors().
 */
class SensitivityFovConverterPage final : public QWidget {
    Q_OBJECT

public:
    explicit SensitivityFovConverterPage(QWidget* parent = nullptr);

    [[nodiscard]] QVariantMap currentInputs() const;

    void setScaleFactors(double horizontalScale, double verticalScale);
    void setCalculationError(const QString& message);
    void clearResults();

Q_SIGNALS:
    void conversionRequested(const QVariantMap& inputs);

private:
    void requestConversion();
    void resetInputs();
    void setStatus(const QString& text, bool success = false, bool error = false);
    void bindIntegerSlider(QSlider* slider, QLineEdit* valueBox);

    QLineEdit* m_sensitivityInput = nullptr;
    QLineEdit* m_multiplierInput = nullptr;

    QSlider* m_verticalFovSlider = nullptr;
    QLineEdit* m_verticalFovValue = nullptr;

    QComboBox* m_aspectRatio = nullptr;
    QComboBox* m_nativeMonitorRatio = nullptr;

    QSlider* m_ads1xSlider = nullptr;
    QLineEdit* m_ads1xValue = nullptr;
    QSlider* m_ads25xSlider = nullptr;
    QLineEdit* m_ads25xValue = nullptr;

    QLabel* m_horizontalOutput = nullptr;
    QLabel* m_verticalOutput = nullptr;
    QLabel* m_statusLabel = nullptr;

    QPushButton* m_calculateButton = nullptr;
};
