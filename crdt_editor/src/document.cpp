#include<src/document.hpp>

std::string Document::render() {
    std::string output;
    visit_visible(ROOT_ID, [&](const ElementID& id) {
        output.push_back(elements.at(id).get_char());
    });
    return output;
}

void Document::visit_visible(const ElementID& node, const std::function<void(const ElementID&)>& visitor) const {
    if (node != ROOT_ID && !elements.at(node).get_deleted()) {
        visitor(node);
    }
    auto it = children.find(node);
    if (it != children.end()) {
        for (const auto& child: it->second) {
            visit_visible(child, visitor);
        }
    }
}

std::string Document::debug_print() {
    return visit_for_testing(ROOT_ID);
}

std::string Document::visit_for_testing(const ElementID& node) {
    std::string output = "";
    if (!elements.at(node).get_deleted()) {
        if (!elements.at(node).get_deleted()) {
            output.append("[" + node.to_String() + ", " + elements.at(node).get_char() + "], ");
        }
    }
    auto it = children.find(node);
    if (it != children.end()) {
        for (const auto& child: it->second) {
            output += visit_for_testing(child);
        }
    }
    return output;
}

void Document::validate() const {
    if (!elements.contains(ROOT_ID)) {
        throw std::runtime_error("ROOT_ID missing from elements");
    }
    if (!children.contains(ROOT_ID)) {
        throw std::runtime_error("ROOT_ID missing from children");
    }
    if (!parent_of.contains(ROOT_ID)) {
        throw std::runtime_error("ROOT_ID missing from parent_of");
    }

    for (const auto& [id, character] : elements) {
        if (!children.contains(id)) {
            throw std::runtime_error("Element exists but has no children entry: " + id.to_String());
        }
        if (!parent_of.contains(id)) {
            throw std::runtime_error("Element exists but has no parent: " + id.to_String());
        }
    }

    for (const auto& [parent, child_list] : children) {
        for (const auto& child : child_list) {
            if (!elements.contains(child)) {
                throw std::runtime_error("Child exists in tree but not elements: " + child.to_String());
            }
            if (!parent_of.contains(child)) {
                throw std::runtime_error("Child has no parent_of entry: " + child.to_String());
            }
            if (parent_of.at(child) != parent) {
                throw std::runtime_error("parent_of disagrees with children map for " + child.to_String());
            }
        }
    }
}


void Document::apply(const Operation& op) {
    std::visit([&](const auto& operation) { 
        using T = std::decay_t<decltype(operation)>;
        if constexpr (std::is_same_v<T, InsertOperation>) {
            this->applyInsert(operation);
        } else if constexpr (std::is_same_v<T, RemoveOperation>) {
            this->applyDelete(operation);
        }
    }, op); 
}

std::optional<ElementID> Document::visible_successor(const ElementID node) const {
    auto next = successor(node);
    while (next && elements.at(*next).get_deleted()) {
        next = successor(*next);
    }
    return next;
}

std::optional<ElementID> Document::successor(const ElementID node) const { //maybe just deal with there not being anything there
    auto child_it = children.find(node);
    if (child_it == children.end()) {
        throw std::runtime_error("successor called on node absent from children: " + node.to_String());
    }

    if (!children.at(node).empty()) {
        return children.at(node)[0];
    }

    ElementID curr = node;

    while (curr != ROOT_ID) {
        ElementID parent = parent_of.at(curr);
        for (int i = 0; i < children.at(parent).size(); i++) { // if node has children
            if (children.at(parent).at(i) == curr) {
                if (i + 1 < children.at(parent).size()) {
                    return children.at(parent).at(i+1);
                }
                break;
            }
        }
        curr = parent;
    }
    return std::nullopt;
}

const ElementID Document::visible_predecessor(const ElementID node) const {
    if (node == ROOT_ID) {
        return ROOT_ID;
    }
    ElementID last = predecessor(node);
    while (elements.at(last).get_deleted()) {
        last = predecessor(last);
    }
    return last;
}

const ElementID Document::predecessor(const ElementID node) const {
    ElementID parent = parent_of.at(node);
    const auto& siblings = children.at(parent);
    // I think always has at least one "sibling" since it exists
    int id = 0;
    while (id < siblings.size()) {
        if (id == siblings.size()) {
            throw std::runtime_error("Node not found among parent's children.");
        }
        if (siblings.at(id) == node) {
            break;
        }
        id++;
    }

    if (siblings.at(0) == node) {
        return parent;
    }

    ElementID previous = siblings.at(id-1);

    while(children.at(previous).size() > 0) {
        previous = children.at(previous).at(children.at(previous).size()-1);
    }
    
    return previous;
}

Document::Document() {
    elements.emplace(ROOT_ID, Character('\0', false));
    children[ROOT_ID] = {};
    parent_of.emplace(ROOT_ID, ROOT_ID);
}

std::pair<size_t, size_t> Document::get_line_column(ElementID target) const {
    size_t line = 0;
    size_t column = 0;
    std::pair<size_t, size_t> result;
    bool found = false;
    visit_visible(ROOT_ID, [&](const ElementID& id) {
        if (found) {
            return;
        }
        if (id == target) {
            result = {line, column};
            found = true;
            return;
        }
        char c = elements.at(id).get_char();
        if (c == '\n') {
            ++line;
            column = 0;
        }
        else {
            ++column;
        }
    });
    if (!found) {
        throw std::runtime_error("Element not found");
    }
    return result;
}

std::optional<ElementID> Document::get_element_at(size_t target_line, size_t target_column) const {

    size_t line = 0;
    size_t column = 0;

    std::optional<ElementID> result;

    visit_visible(ROOT_ID, [&](const ElementID& id) {
        if (result) {
            return;
        }
        if (line == target_line && column == target_column) {
            result = id;
            return;
        }
        char c = elements.at(id).get_char();
        if (c == '\n') {
            ++line;
            column = 0;
        }
        else {
            ++column;
        }
    });
    return result;
}

std::optional<ElementID> Document::get_anchor_at(size_t target_line, size_t target_column) const
{
    size_t line = 0;
    size_t column = 0;

    ElementID previous = ROOT_ID;
    std::optional<ElementID> result;

    visit_visible(ROOT_ID, [&](const ElementID& id) {
        if (result) {
            return;
        }

        // The cursor belongs immediately before this element.
        if (line == target_line && column == target_column) {
            result = previous;
            return;
        }

        previous = id;

        char c = elements.at(id).get_char();

        if (c == '\n') {
            ++line;
            column = 0;
        }
        else {
            ++column;
        }
    });

    // Cursor at the end of the document.
    if (!result && line == target_line && column == target_column) {
        result = previous;
    }

    return result;
}

char Document::get_character(ElementID id) const {
    return elements.at(id).get_char();
}

std::optional<size_t> Document::get_line_length(size_t target_line) const {
    size_t current_line = 0;
    size_t length = 0;

    visit_visible(ROOT_ID, [&](const ElementID& id) {
        char c = elements.at(id).get_char();

        if (current_line > target_line) {
            return;
        }
        if (current_line == target_line) {
            if (c == '\n') {
                ++current_line;
                return;
            }
            ++length;
            return;
        }

        if (c == '\n') {
            ++current_line;
        }
    });

    if (current_line == target_line) {
        return length;
    }
    // If we crossed the target line, it existed.
    if (current_line > target_line) {
        return length;
    }

    return std::nullopt;
}

std::pair<size_t, size_t> Document::get_cursor_position(const ElementID& anchor) const {
    if (anchor == ROOT_ID) {
        return {0, 0};
    }
    auto [line, column] = get_line_column(anchor);
    char c = get_character(anchor);
    if (c == '\n') {
        return {line + 1, 0};
    }
    return {line, column + 1};
}

size_t Document::get_max_line_length() const {
    size_t max_length = 0;
    size_t current_length = 0;

    visit_visible(ROOT_ID, [&](const ElementID& id) {
        char c = elements.at(id).get_char();
        if (c == '\n') {
            max_length = std::max(max_length, current_length);
            current_length = 0;
        } else {
            ++current_length;
        }
    });
    max_length = std::max(max_length, current_length);

    return max_length;
}


std::pair<size_t, size_t> Document::clamp_position(size_t requested_line, size_t requested_column) const {
    size_t line_count = get_line_count();
    if (line_count == 0) {
        return {0, 0};
    }
    size_t line = std::min(requested_line, line_count -1);
    size_t length = get_line_length(line).value_or(0);
    size_t column = std::min(requested_column, length);

    return {line, column};
}

size_t Document::get_line_count() const {
    size_t line = 1;

    visit_visible(ROOT_ID, [&](const ElementID& id) {
        char c = elements.at(id).get_char();
        if (c == '\n') {
            ++line;
        }
    });
    return line;
}



void Document::applyInsert(const InsertOperation& oper) {
    // std:: cout << "before insert ordering = " << ordered_results_string() << std::endl;
    if (elements.contains(oper.get_id())) {
        return;
    }

    // parent has not arrived yet.
    if (!elements.contains(oper.get_parent())) {
        if (buffered_insert_ids.insert(oper.get_id()).second) { // so don't have duplicate children
            buffered_inserts[oper.get_parent()].push_back(oper);
        }
        
        std::cout << "BUFFERED INSERT: " << oper.get_id().to_String() << " waiting for " << oper.get_parent().to_String() << std::endl;
        return;
    }

    // parent exists, so insert the element
    bool already_deleted = buffered_deletes.erase(oper.get_id()) > 0;
    Character cha = Character(oper.get_char(), already_deleted);

    auto [it, inserted] = elements.emplace(oper.get_id(), cha); // if this succeeds, then do the rest otherwise don't.
        
    if (inserted) {
        buffered_insert_ids.erase(oper.get_id());
    }
        
    if (!inserted) {
        return;
    }

    parent_of.emplace(oper.get_id(), oper.get_parent());

    auto& siblings = children[oper.get_parent()];

    siblings.push_back(oper.get_id());
    children.try_emplace(oper.get_id());



    /*
    Siblings are ordered by descending ElementID. Elements sharing a parent represent concurrent/alternative
    insertions at the same logical position. Descending ElementID provides deterministic ordering across replicas.
    
    Sequential typing does not normally create siblings because the cursor advances to the newly inserted element
    and the next insertion therefore becomes its child.
    */
    std::sort(siblings.begin(), siblings.end(),[](const ElementID& a, const ElementID& b) {return a > b;});
    //std::sort(siblings.begin(), siblings.end()) 

    std::cout << "Children of " << oper.get_parent().to_String() << ":\n";

    for (const auto& child : siblings) {
        std::cout << "  " << child.to_String() << " -> '" << elements.at(child).get_char() << "'\n";
    }

    // find operations waiting for this newly-arrived element
    auto waiting = buffered_inserts.find(oper.get_id());

    if (waiting == buffered_inserts.end()) {
        return;
    }

    std::vector<InsertOperation> waiting_children = std::move(waiting->second);
        
    // remove from the buffer
    buffered_inserts.erase(waiting);

    // apply every child waiting for the element
    for (const InsertOperation& child: waiting_children) {
        std::cout << "UNBUFFERED INSERT: "
            << child.get_id().to_String()
            << " with parent "
            << child.get_parent().to_String()
            << std::endl;
        applyInsert(child);
    }

    // validation testing
    for (const auto& [missing_parent, operations] : buffered_inserts) {
        if (elements.contains(missing_parent)) {
            throw std::runtime_error(
                "Buffered operations exist for a parent that is already present"
            );
        }

        for (const auto& oper : operations) {
            if (oper.get_parent() != missing_parent) {
                throw std::runtime_error(
                    "Buffered operation stored under wrong parent key"
                );
            }

            if (elements.contains(oper.get_id())) {
                throw std::runtime_error(
                    "Operation is both buffered and already inserted"
                );
            }
        }
    }
}
    
void Document::applyDelete(const RemoveOperation& oper) {
    if (elements.contains(oper.get_target())) {
        elements.find(oper.get_target())->second.set_deleted(true); // running this twice doesn't change the flag back so its fine?
    } else {
        std::cout << "BUFFERED DELETE: " << oper.get_target().to_String() << std::endl;
        buffered_deletes.insert(oper.get_target());
    }
};


std::unordered_map<ElementID, Character> Document::get_elements() const {
    return elements;
}
