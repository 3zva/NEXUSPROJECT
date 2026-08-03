#pragma once

#include <QWidget>

class QLineEdit;
class QPushButton;
class QLabel;
class NexusProgressView;
class QStackedLayout;

class PathSelectionPage final : public QWidget {
    Q_OBJECT

public:
    explicit PathSelectionPage(QWidget* parent = nullptr);
    [[nodiscard]] QString selectedPath() const;
    void setSelectedPath(const QString& path);
    void showStatus(const QString& message, bool error = false);
    void beginLoading();
    void setLoadingWaiting();
    void setLoadingGameDetected(qint64 pid, const QString& executableName);
    void setLoadingClientReady();
    void finishLoading();
    void failLoading(const QString& message);
    void resetLoading();

Q_SIGNALS:
    void loadRequested(const QString& path);

private:
    QStackedLayout* m_stack = nullptr;
    QLineEdit* m_pathEdit = nullptr;
    QPushButton* m_loadButton = nullptr;
    QLabel* m_status = nullptr;
    NexusProgressView* m_loadingView = nullptr;
};
