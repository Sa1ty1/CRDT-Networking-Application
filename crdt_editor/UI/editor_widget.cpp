#include <UI/editor_widget.hpp>


EditorWidget::EditorWidget(EditorSession& session, NetworkClient& network, QWidget* parent): QWidget(parent), session(session), network(network), network_timer(new QTimer(this)) {
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(800, 600);
    setFont(QFont("Consolas"));

    connect(network_timer, &QTimer::timeout, this, [this]() {
        this->network.poll();
        this->session.flush_incoming();
        update();
    });
    network_timer->start(10);
}

QColor EditorWidget::cursor_color(const std::string& client_id) {
    static const std::vector<QColor> colors = {
        Qt::red,
        Qt::blue,
        Qt::darkGreen,
        Qt::magenta,
        Qt::darkCyan,
        Qt::darkYellow
    };
    std::hash<std::string> hash;
    return colors[hash(client_id) % colors.size()];
}


void EditorWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    QFontMetrics metrics(font());

    draw_document(painter, metrics);
    draw_local_cursor(painter, metrics);
    draw_remote_cursors(painter, metrics);
}

void EditorWidget::keyPressEvent(QKeyEvent* event) {

    std::cout << "Qt key = " << event->key() << ", text = [" << event->text().toStdString() << "]\n";

    switch(event->key()) {
        case Qt::Key_Left:
            session.handle_editor_command(EditorCommand(MoveLeft));
            break;
        case Qt::Key_Right:
            session.handle_editor_command(EditorCommand(MoveRight));
            break;
        case Qt::Key_Up:
            session.handle_editor_command(EditorCommand(MoveUp));
            break;
        case Qt::Key_Down:
            session.handle_editor_command(EditorCommand(MoveDown));
            break;
        case Qt::Key_Backspace:
            session.handle_editor_command(EditorCommand(Backspace));
            break;
        case Qt::Key_Delete:
            session.handle_editor_command(EditorCommand(DeleteForward));
            break;
        case Qt::Key_Return:
            session.handle_editor_command(EditorCommand(Enter));
            break;
        case Qt::Key_Enter:
            session.handle_editor_command(EditorCommand(Enter));
            break;
        default:
            // text keys
            if (!event->text().isEmpty()) {
                QString text = event->text();
                for (QChar character : text) {
                    session.handle_editor_command(EditorCommand(InsertCharacter, character.toLatin1())); // deal with unicode later
                }
            }
            break;
    }

    // Send document operations before advertising the new cursor position.
    if (network.get_state() == NetworkClientState::LIVE) {
        network.send_outgoing_operations();
    }

    send_cursor_update();

    event->accept();
    update();
}

void EditorWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) { // will deal with these later
        return;
    }

    setFocus();

    auto [requested_line, requested_column] = screen_to_document_position(event->position().toPoint());

    auto [line, column] = session.get_doc().clamp_position(requested_line, requested_column);

    std::cout << "Clicked: " << line << ", column: " << column << std::endl;
    auto anchor = session.get_doc().get_anchor_at(line, column);

    if (anchor) {
        session.get_cursor().set_position(*anchor, session.get_doc());
        send_cursor_update();
    }
    event->accept();
    update();
}

std::pair<size_t, size_t> EditorWidget::screen_to_document_position(QPoint position) const {
    QFontMetrics metrics(font());

    int char_width = metrics.horizontalAdvance('M');
    int line_height = metrics.height();

    int relative_x = position.x() - TEXT_X;

    // TEXT_Y is the text baseline, so the top of the first
    // line is TEXT_Y - ascent().
    int relative_y = position.y() - TEXT_Y;

    if (relative_x < 0 || relative_y < 0) {
        return {0, 0};
    }

    size_t line = relative_y / line_height;
    size_t column = relative_x / char_width;

    return {line, column};
}

QPoint EditorWidget::document_to_screen(size_t line, size_t column) const {
    QFontMetrics metrics(font());
    int char_width = metrics.horizontalAdvance('M');
    int line_height = metrics.height();
    int x = TEXT_X + column * char_width;
    int y = TEXT_Y + line * line_height;
    return QPoint(x, y);
}

void EditorWidget::send_cursor_update() {
    if (network.get_state() != NetworkClientState::LIVE) {
        return;
    }
    network.send_cursor_update(session.get_cursor().get_anchor());
}

void EditorWidget::draw_document(QPainter& painter, const QFontMetrics& metrics) {
    int char_width = metrics.horizontalAdvance('M');
    int line_height = metrics.height();

    std::string text = session.get_doc().render();

    int current_x = TEXT_X;
    int current_y = TEXT_Y;

    for (char c : text) {
        if (c == '\n') {
            current_x = TEXT_X;
            current_y += line_height;
        } else {
            painter.drawText(current_x, current_y + metrics.ascent(), QString(QChar(c)));
            current_x += char_width;
        }
    }
}

void EditorWidget::draw_local_cursor(QPainter& painter, const QFontMetrics& metrics) {

    auto [line, col] = session.get_doc().get_cursor_position(session.get_cursor().get_anchor());
    //auto [line, col] = session.get_cursor().get_line_column(session.get_doc());
    QPoint position = document_to_screen(line, col);

    QPen cursor_pen(Qt::white);
    cursor_pen.setWidth(2);
    painter.setPen(cursor_pen);

    painter.drawLine(position.x(), position.y(), position.x(), position.y() + metrics.height());
}

void EditorWidget::draw_remote_cursors(QPainter& painter, const QFontMetrics& metrics) {

    for (const auto& [client_id, anchor] : session.get_remote_cursors()) {
        try {
            auto [line, col] = session.get_doc().get_cursor_position(anchor);
            QPoint screen_position = document_to_screen(line, col);

            painter.setPen(cursor_color(client_id));

            painter.drawLine(screen_position.x(), screen_position.y(), screen_position.x(), screen_position.y() + metrics.height());
            // label the cursor
            QString label = QString::fromStdString(client_id);
            QRect label_rect(screen_position.x() - (metrics.horizontalAdvance(label)/2), screen_position.y() + metrics.height(), metrics.horizontalAdvance(label) + 8, metrics.height());
            painter.fillRect(label_rect, cursor_color(client_id));
            painter.setPen(Qt::white);
            painter.drawText(label_rect, Qt::AlignCenter, label);
        } catch (const std::runtime_error&) {
            // Remote cursor references an element we don't have
            continue; // this should prevent crashing from this issue that probably shouldnt crash everything
        }
    }
}