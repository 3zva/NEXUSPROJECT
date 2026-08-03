#pragma once

#include <QWidget>

class QColor;
class QLabel;
class QProgressBar;
class QPushButton;
class QPropertyAnimation;

enum class NexusProgressMode {
    GameLaunch,
    Update
};

/**
 * Shared NEXUS progress surface used by full-window client progress screens.
 *
 * This class is presentation-only. It does not launch processes, download
 * files, install updates, or interact with Rainbow Six Siege.
 */
class NexusProgressView final : public QWidget {
    Q_OBJECT

public:
    explicit NexusProgressView(
        NexusProgressMode mode,
        QWidget* parent = nullptr
    );

    void setMode(NexusProgressMode mode);
    void reset();

    void setTitle(const QString& title);
    void setSubtitle(const QString& subtitle);
    void setStage(const QString& stage);
    void setStatus(const QString& status);
    void setDetail(const QString& detail);
    void setProgress(int percent, bool animate = true);
    void setComplete(const QString& message);
    void setError(const QString& message);

    void setActionVisible(bool visible);
    void setActionText(const QString& text);
    void setActionEnabled(bool enabled);

    [[nodiscard]] int progress() const;
    [[nodiscard]] NexusProgressMode mode() const;

Q_SIGNALS:
    void actionRequested();

private:
    void buildUi();
    void applyModeText();
    void refreshPercentLabel(int value);
    void setStatusColor(const QColor& color);

    NexusProgressMode m_mode;
    QLabel* m_eyebrowLabel = nullptr;
    QLabel* m_logoLabel = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_subtitleLabel = nullptr;
    QLabel* m_stageLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_detailLabel = nullptr;
    QLabel* m_percentLabel = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QPushButton* m_actionButton = nullptr;
    QPropertyAnimation* m_progressAnimation = nullptr;
};
