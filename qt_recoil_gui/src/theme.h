#pragma once

#include <QColor>
#include <QFont>
#include <QIcon>
#include <QPixmap>
#include <QString>

namespace NexusTheme {
inline const QColor Background{"#070A12"};
inline const QColor Sidebar{"#0B0F1A"};
inline const QColor Surface{"#0E1320"};
inline const QColor Surface2{"#131A2A"};
inline const QColor Surface3{"#182136"};
inline const QColor Border{"#252D40"};
inline const QColor Text{"#F7F9FF"};
inline const QColor Muted{"#98A2B7"};
inline const QColor Subtle{"#68738B"};
inline const QColor Accent{"#765BFF"};
inline const QColor AccentHover{"#6848F4"};
inline const QColor AccentBright{"#A898FF"};
inline const QColor AccentSoft{"#28204D"};
inline const QColor Success{"#4ED49A"};
inline const QColor Danger{"#FF6B82"};
inline const QColor Warning{"#F4C96B"};
inline constexpr int SidebarWidth = 232;
inline constexpr int ContentPadding = 28;

inline QFont font(int pointSize, QFont::Weight weight = QFont::Normal) {
    QFont result(QStringLiteral("Segoe UI"), pointSize);
    result.setWeight(weight);
    return result;
}

inline QIcon icon(const QString& fileName) {
    return QIcon(QStringLiteral(":/assets/") + fileName);
}

inline QPixmap pixmap(const QString& fileName, int width, int height) {
    QPixmap image(QStringLiteral(":/assets/") + fileName);
    return image.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

inline QString globalStyleSheet() {
    return QStringLiteral(R"QSS(
        QWidget {
            color: #F7F9FF;
            background: transparent;
            font-family: "Segoe UI";
            font-size: 12px;
        }
        QMainWindow, QWidget#appRoot {
            background: #070A12;
        }
        QFrame#sidebar {
            background: #0B0F1A;
            border: none;
        }
        QFrame[card="true"] {
            background: #0E1320;
            border: 1px solid #252D40;
            border-radius: 16px;
        }
        QFrame[card="elevated"] {
            background: #131A2A;
            border: 1px solid #252D40;
            border-radius: 16px;
        }
        QLabel[muted="true"] { color: #98A2B7; }
        QLabel[subtle="true"] { color: #68738B; }
        QLabel[accent="true"] { color: #A898FF; }
        QLabel[success="true"] { color: #4ED49A; }
        QLabel[rapidFireEnabled="false"] {
            color: #FF6B82;
            background: #21121A;
            border: 1px solid #5E2B3A;
            border-radius: 10px;
        }
        QLabel[rapidFireEnabled="true"] {
            color: #4ED49A;
            background: #10231C;
            border: 1px solid #245C48;
            border-radius: 10px;
        }
        QPushButton {
            min-height: 34px;
            padding: 0 14px;
            border-radius: 9px;
            border: 1px solid #252D40;
            background: #182136;
            color: #F7F9FF;
            font-weight: 600;
        }
        QPushButton:hover { background: #28204D; border-color: #765BFF; }
        QPushButton:pressed { background: #20183F; }
        QPushButton:disabled { color: #68738B; background: #101521; border-color: #1D2534; }
        QPushButton[accentButton="true"] {
            background: #765BFF;
            border-color: #765BFF;
            color: white;
        }
        QPushButton[accentButton="true"]:hover { background: #6848F4; }
        QPushButton[dangerButton="true"] {
            background: #FF6B82;
            border-color: #FF6B82;
            color: white;
        }
        QPushButton[dangerButton="true"]:hover { background: #E5536A; }
        QPushButton[stepperButton="true"] {
            min-height: 34px;
            max-height: 34px;
            min-width: 38px;
            max-width: 38px;
            padding: 0;
            font-size: 17px;
            font-weight: 700;
            background: #182136;
            border-color: #303A50;
        }
        QPushButton[stepperButton="true"]:hover {
            background: #28204D;
            border-color: #765BFF;
            color: #A898FF;
        }
        QPushButton[navButton="true"] {
            text-align: left;
            padding-left: 14px;
            min-height: 42px;
            border: 1px solid transparent;
            background: transparent;
            color: #98A2B7;
        }
        QPushButton[navButton="true"]:hover {
            background: #182136;
            color: #F7F9FF;
        }
        QPushButton[navActive="true"] {
            background: #28204D;
            border-color: #765BFF;
            color: #F7F9FF;
        }
        QLineEdit, QComboBox, QPlainTextEdit {
            min-height: 34px;
            padding: 0 10px;
            background: #0E1320;
            border: 1px solid #252D40;
            border-radius: 8px;
            selection-background-color: #765BFF;
        }
        QPlainTextEdit { padding: 10px; }
        QLineEdit:focus, QComboBox:focus, QPlainTextEdit:focus { border-color: #765BFF; }
        QLineEdit[valueBox="true"] {
            background: #090D17;
            border: 1px solid #39445A;
            color: #F7F9FF;
            font-size: 13px;
            font-weight: 700;
            padding: 0 8px;
        }
        QComboBox::drop-down { border: none; width: 28px; }
        QComboBox QAbstractItemView {
            background: #131A2A;
            border: 1px solid #252D40;
            selection-background-color: #28204D;
            outline: none;
        }
        QCheckBox { spacing: 10px; color: #F7F9FF; }
        QCheckBox::indicator {
            width: 34px;
            height: 18px;
            border-radius: 9px;
            background: #39445A;
            border: 1px solid #4A5670;
        }
        QCheckBox::indicator:checked {
            background: #765BFF;
            border-color: #765BFF;
        }
        QScrollArea { border: none; background: transparent; }
        QScrollBar:vertical {
            width: 10px;
            background: transparent;
            margin: 2px;
        }
        QScrollBar::handle:vertical {
            background: #252D40;
            border-radius: 5px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover { background: #765BFF; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        QMessageBox {
            background: #0E1320;
        }
        QMessageBox QLabel {
            color: #F7F9FF;
            background: transparent;
            font-size: 12px;
        }
        QMessageBox QPushButton {
            min-width: 86px;
            min-height: 34px;
            background: #182136;
            border: 1px solid #252D40;
            border-radius: 9px;
            color: #F7F9FF;
            font-weight: 600;
        }
        QMessageBox QPushButton:hover {
            background: #28204D;
            border-color: #765BFF;
        }
        QToolTip {
            background: #131A2A;
            color: #F7F9FF;
            border: 1px solid #252D40;
            padding: 6px;
        }
    )QSS");
}
}
