#pragma once

#include <QWidget>

class QLineEdit;
class QPushButton;
class QLabel;

class PathSelectionPage final : public QWidget {
    Q_OBJECT

public:
    explicit PathSelectionPage(QWidget* parent = nullptr);
    [[nodiscard]] QString selectedPath() const;
    void setSelectedPath(const QString& path);
    void showStatus(const QString& message, bool error = false);

Q_SIGNALS:
    void loadRequested(const QString& path);

private:
    QLineEdit* m_pathEdit = nullptr;
    QPushButton* m_loadButton = nullptr;
    QLabel* m_status = nullptr;
};
