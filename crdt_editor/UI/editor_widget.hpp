#pragma once
#include <QWidget>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPoint>
#include <QTimer>
#include <QColor>
#include <utility>
#include <network/editor_session.hpp>
#include <network/network_client.hpp>

struct CursorStyle {
    QColor color;
};

class EditorWidget : public QWidget {
public:
    explicit EditorWidget(EditorSession& session, NetworkClient& network, QWidget* parent = nullptr);
    // void set_remote_cursor(std::string client_id, ElementID position);
protected:
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    QColor cursor_color(const std::string& client_id);
    QPoint document_to_screen(size_t line, size_t column) const;

private:
    EditorSession& session;
    static constexpr int TEXT_X = 5;
    static constexpr int TEXT_Y = 5;
    QTimer* network_timer;
    NetworkClient& network;
    std::pair<size_t, size_t> screen_to_document_position(QPoint position) const;
    void draw_document(QPainter& painter, const QFontMetrics& metrics);
    void draw_local_cursor(QPainter& painter, const QFontMetrics& metrics);
    void draw_remote_cursors(QPainter& painter, const QFontMetrics& metrics);
    void send_cursor_update();
};