#include <UI/editor_widget.hpp>


EditorWidget::EditorWidget(EditorSession& session, NetworkClient& network, QWidget* parent): QWidget(parent), session(session), network(network), network_timer(new QTimer(this)), cursor_blink_timer(new QTimer(this)) {
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(800, 600);
    setFont(QFont("Consolas"));

    connect(network_timer, &QTimer::timeout, this, [this]() {
        this->network.poll();
        this->session.flush_incoming();
        update();
    });
    network_timer->start(10);

    cursor_blink_timer->setInterval(500);
    connect(cursor_blink_timer, &QTimer::timeout, this, [this]() {
        cursor_visible = !cursor_visible;
        update();
    });
    cursor_blink_timer->start();
    setCursor(Qt::IBeamCursor); // the nice "I" cursor for the mouse
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

    draw_selection(painter, metrics);
    draw_document(painter, metrics);
    draw_remote_cursors(painter, metrics);
    draw_local_cursor(painter, metrics);
}

void EditorWidget::keyPressEvent(QKeyEvent* event) {

    // std::cout << "Qt key = " << event->key() << ", text = [" << event->text().toStdString() << "]\n";

    bool shift = event->modifiers() & Qt::ShiftModifier;

    switch(event->key()) {
        case Qt::Key_Left:
            session.handle_editor_command(EditorCommand(
                shift ? ShiftLeft : MoveLeft));
            break;
        case Qt::Key_Right:
            session.handle_editor_command(EditorCommand(
                shift ? ShiftRight : MoveRight));
            break;
        case Qt::Key_Up:
            session.handle_editor_command(EditorCommand(
                shift ? ShiftUp : MoveUp));
            break;
        case Qt::Key_Down:
            session.handle_editor_command(EditorCommand(
                shift ? ShiftDown: MoveDown));
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
    reset_cursor_blink();
    clamp_viewport();
    ensure_cursor_visible();

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

    auto anchor = session.get_doc().get_anchor_at(line, column);

    if (anchor) {
        auto& cursor = session.get_cursor();
        cursor.set_position(*anchor, session.get_doc());
        //cursor.start_selection();
        selecting = true;
        send_cursor_update();
        reset_cursor_blink();
        ensure_cursor_visible();
    }

    event->accept();
    update();
}

void EditorWidget::wheelEvent(QWheelEvent* event) {
    int delta = event->angleDelta().y();
    if (delta > 0) {
        viewport.scroll_lines(-3); // change 3 for different scroll feel
    } else if (delta < 0) {
        viewport.scroll_lines(3);
    }
    clamp_viewport();
    update();
    event->accept();
}

void EditorWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);

    clamp_viewport();
    ensure_cursor_visible();
}

void EditorWidget::mouseMoveEvent(QMouseEvent* event) {
    if (!selecting) {
        return;
    }
    auto [requested_line, requested_column] = screen_to_document_position(event->position().toPoint());
    auto [line, column] = session.get_doc().clamp_position(requested_line, requested_column);
    auto anchor = session.get_doc().get_anchor_at(line, column);

    if (anchor) {
        if (!session.get_cursor().has_selection()) {
            session.get_cursor().start_selection();
        }
        session.get_cursor().set_anchor(*anchor);
        reset_cursor_blink();
        ensure_cursor_visible();
        update();
    }
    event->accept();
}

void EditorWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        selecting = false;
    }
    event->accept();
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

    // size_t line = relative_y / line_height;
    // size_t column = relative_x / char_width;
    size_t line = viewport.get_first_line() + static_cast<size_t>(relative_y / line_height);
    size_t column = viewport.get_first_column() + static_cast<size_t>(relative_x / char_width);

    return {line, column};
}

QPoint EditorWidget::document_to_screen(size_t line, size_t column) const {
    QFontMetrics metrics(font());
    int char_width = metrics.horizontalAdvance('M');
    int line_height = metrics.height();
    int x = TEXT_X + (static_cast<int>(column) - static_cast<int>(viewport.get_first_column())) * char_width;
    int y = TEXT_Y + (static_cast<int>(line) - static_cast<int>(viewport.get_first_line())) * line_height;
    
    // int x = TEXT_X + static_cast<int>(line_column_difference(column, viewport.get_first_column())) * char_width;
    // int y = TEXT_Y + static_cast<int>(line_difference(line, viewport.get_first_line())) * line_height;
    return QPoint(x,y);
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

    int current_x = TEXT_X - static_cast<int>(viewport.get_first_column()) * char_width;
    int current_y = TEXT_Y - static_cast<int>(viewport.get_first_line()) * line_height;

    for (char c : text) {
        if (c == '\n') {
            current_x = TEXT_X - static_cast<int>(viewport.get_first_column()) * char_width;
            current_y += line_height;
        } else {
            painter.drawText(current_x, current_y + metrics.ascent(), QString(QChar(c)));
            current_x += char_width;
        }
    }
}

void EditorWidget::draw_local_cursor(QPainter& painter, const QFontMetrics& metrics) {

    if (!cursor_visible) {
        return;
    }
    const ElementID& anchor = session.get_cursor().get_anchor();
    if (anchor != ROOT_ID && session.get_doc().get_elements().contains(anchor) && session.get_doc().get_character(anchor)) {
        // what am I doing here
    }
    auto [line, col] = session.get_doc().get_cursor_position(anchor);
    //auto [line, col] = session.get_cursor().get_line_column(session.get_doc());
    QPoint position = document_to_screen(line, col);

    QPen cursor_pen(Qt::white);
    cursor_pen.setWidth(2);
    cursor_pen.setCapStyle(Qt::RoundCap);
    painter.setPen(cursor_pen);

    painter.drawLine(position.x(), position.y(), position.x(), position.y() + metrics.height());
}

void EditorWidget::draw_remote_cursors(QPainter& painter, const QFontMetrics& metrics) {

    for (const auto& [client_id, anchor] : session.get_remote_cursors()) {
        
        try {
            auto resolved = session.get_doc().resolve_cursor_anchor(anchor);
            if (!resolved) {
                std::cerr << "Cannot resolve remote cursor " << client_id << " at " << anchor.to_String() << '\n';
                continue;
            }
            auto [line, col] = session.get_doc().get_cursor_position(*resolved);
            // auto [line, col] = session.get_doc().get_cursor_position(anchor);
            QPoint screen_position = document_to_screen(line, col);

            painter.setPen(cursor_color(client_id));
            painter.drawLine(screen_position.x(), screen_position.y(), screen_position.x(), screen_position.y() + metrics.height());
            // label the cursor
            QString label = QString::fromStdString(client_id);
            QRect label_rect(screen_position.x() - (metrics.horizontalAdvance(label)/2), screen_position.y() + metrics.height(), metrics.horizontalAdvance(label) + 8, metrics.height());
            painter.fillRect(label_rect, cursor_color(client_id));
            painter.setPen(Qt::white);
            painter.drawText(label_rect, Qt::AlignCenter, label);
        } catch (const std::runtime_error& e) {
            std::cerr << "Failed to render remote cursor for " << client_id << ": " << e.what() << '\n';
            // Remote cursor references an element we don't have
            continue; // this should prevent crashing from this issue that probably shouldnt crash everything
        }
    }
}

void EditorWidget::reset_cursor_blink() {
    cursor_visible = true;
    cursor_blink_timer->start();
    update();
}

void EditorWidget::ensure_cursor_visible() {
    auto [line, column] = session.get_doc().get_cursor_position(session.get_cursor().get_anchor());
    QFontMetrics metrics(font());
    int line_height = metrics.height();

    if (line_height <= 0) {
        return;
    }

    size_t visible_lines = static_cast<size_t>(std::max(1, height() - TEXT_Y) / line_height);
    size_t visible_columns = static_cast<size_t>(std::max(1, width() - TEXT_X) / metrics.horizontalAdvance('M'));
    
    if (line < viewport.get_first_line()) {
        viewport.set_first_line(line);
    } else if (line >= viewport.get_first_line() + visible_lines) {
        viewport.set_first_line(line - visible_lines + 1);
    }

    if (column < viewport.get_first_column()) {
        viewport.set_first_column(column);
    } else if (column >= viewport.get_first_column() + visible_columns) {
        viewport.set_first_column(column - visible_columns + 1);
    }
}

void EditorWidget::clamp_viewport() {
    QFontMetrics metrics(font());

    int line_height = metrics.height(); 
    int char_width = metrics.horizontalAdvance('M');

    if (line_height <= 0 || char_width <= 0) {
        return;
    }

    size_t visible_lines = static_cast<int>(std::max(1, height() - TEXT_Y) / line_height); 
    size_t visible_columns = static_cast<int>(std::max(1, width() - TEXT_X) / char_width);

    size_t document_lines = session.get_doc().get_line_count();
    size_t document_columns = session.get_doc().get_max_line_length();

    viewport.clamp(document_lines, visible_lines, document_columns, visible_columns);
}


void EditorWidget::draw_selection(QPainter& painter, const QFontMetrics& metrics) {
    auto range = session.get_cursor().normalized_range(session.get_doc());
    if (!range) {
        return;
    }
    auto selected = session.get_doc().get_visible_range(*range);
    int char_width = metrics.horizontalAdvance('M');
    int line_height = metrics.height();

    for (const auto& id : selected) {
        char c = session.get_doc().get_character(id);
        if (c == '\n') {
            continue;
        }
        auto [line, column] = session.get_doc().get_cursor_position(session.get_doc().visible_predecessor(id));
        QPoint position = document_to_screen(line, column);
        painter.fillRect(position.x(), position.y(), char_width, line_height, selection_color);
    }
}
