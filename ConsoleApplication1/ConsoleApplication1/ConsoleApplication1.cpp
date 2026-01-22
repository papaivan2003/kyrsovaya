#include <iostream>
#include <fstream>
#include <regex>
#include <string>
#include <vector>
#include <stack>
#include <sstream>
#include <stdexcept>
#include <memory>
#include <set>
#include <tuple>
#include <map>

// Структура для токенов
struct Token {
    int line_number;
    std::string type;
    std::string value;
};

// Узел синтаксического дерева
struct Node {
    std::string value;
    std::vector<std::shared_ptr<Node>> children;

    explicit Node(const std::string& value) : value(value) {}

    void add_child(std::shared_ptr<Node> child) {
        children.push_back(child);
    }

    void print(int level = 0) const {
        // Добавляем отступы в зависимости от уровня вложенности
        std::cout << std::string(level * 2, ' ') << value << std::endl;
        for (const auto& child : children) {
            child->print(level + 1); // Увеличиваем уровень вложенности для дочерних узлов
        }
    }

};

// Шаблоны токенов
const std::vector<std::pair<std::regex, std::string>> TOKEN_REGEX = {
    {std::regex(R"(\btrue\b|\bfalse\b)"), "Логическая константа"},
    {std::regex(R"(\b(or|xor|and|not)\b)"), "Логический оператор"},
    {std::regex(R"(:=)"), "Оператор присваивания"},
    {std::regex(R"([a-zA-Z_]\w*)"), "Идентификатор"},
    {std::regex(R"(\()"), "Открывающая скобка"},
    {std::regex(R"(\))"), "Закрывающая скобка"},
    {std::regex(R"(;)"), "Точка с запятой"}
};

std::regex INVALID_IDENTIFIER_REGEX(R"(\b\d\w*)");

// Удаление комментариев
std::string remove_comments(const std::string& line, int line_number) {
    auto open_braces = std::count(line.begin(), line.end(), '{');
    auto close_braces = std::count(line.begin(), line.end(), '}');

    if (open_braces != close_braces) {
        throw std::runtime_error("Незакрытый комментарий в строке " + std::to_string(line_number));
    }

    return std::regex_replace(line, std::regex(R"(\{.*?\})"), "");
}

// Лексический анализатор
std::vector<Token> tokenize(const std::string& line, int line_number) {
    std::vector<Token> tokens;
    size_t pos = 0;

    while (pos < line.length()) {
        bool matched = false;
        for (const auto& [pattern, token_type] : TOKEN_REGEX) {
            std::smatch match;
            std::string subline = line.substr(pos);
            if (std::regex_search(subline, match, pattern) && match.position() == 0) {
                tokens.push_back({ line_number, token_type, match.str() });
                pos += match.length();
                matched = true;
                break;
            }
        }
        if (!matched) pos++; // Пропускаем неопознанные символы
    }

    return tokens;
}

// Разделение токенов на выражения
std::vector<std::vector<Token>> split_expressions(const std::vector<Token>& tokens) {
    std::vector<std::vector<Token>> expressions;
    std::vector<Token> current_expression;

    for (const auto& token : tokens) {
        current_expression.push_back(token);
        if (token.type == "Точка с запятой") {
            expressions.push_back(current_expression);
            current_expression.clear();
        }
    }

    if (!current_expression.empty()) {
        throw std::runtime_error("Незавершённое выражение без точки с запятой.");
    }

    return expressions;
}

// Синтаксический анализатор
class SyntaxAnalyzer {
private:
    std::vector<Token> tokens;
    size_t current_index;

    Token current_token() const {
        if (current_index < tokens.size()) {
            return tokens[current_index];
        }
        return Token{ -1, "", "" }; // Пустой токен
    }

    void advance() {
        if (current_index < tokens.size()) {
            ++current_index;
        }
    }

    void expect(const std::string& expected_type) {
        if (current_token().type != expected_type) {
            throw std::runtime_error("Ожидался токен типа " + expected_type + ", но получено: " + current_token().type);
        }
        advance();
    }

    std::shared_ptr<Node> parse_E() {
        auto node = std::make_shared<Node>("E");
        auto child = parse_D();
        node->add_child(child);
        expect("Точка с запятой");
        return node;
    }

    std::shared_ptr<Node> parse_D() {
        auto node = std::make_shared<Node>("D");
        if (current_token().type == "Идентификатор") {
            auto id_node = std::make_shared<Node>(current_token().value);
            node->add_child(id_node);
            advance();
            if (current_token().type == "Оператор присваивания") {
                auto assign_node = std::make_shared<Node>(current_token().value);
                node->add_child(assign_node);
                advance();
                auto child = parse_A();
                node->add_child(child);
            }
            else {
                throw std::runtime_error("Ожидался оператор присваивания := после идентификатора.");
            }
        }
        else {
            auto child = parse_A();
            node->add_child(child);
        }
        return node;
    }

    std::shared_ptr<Node> parse_A() {
        auto node = std::make_shared<Node>("A");
        auto child = parse_B();
        node->add_child(child);
        while (current_token().value == "or" || current_token().value == "xor") {
            auto op_node = std::make_shared<Node>(current_token().value);
            node->add_child(op_node);
            advance();
            auto right_child = parse_B();
            node->add_child(right_child);
        }
        return node;
    }

    std::shared_ptr<Node> parse_B() {
        auto node = std::make_shared<Node>("B");
        auto child = parse_C();
        node->add_child(child);
        while (current_token().value == "and") {
            auto op_node = std::make_shared<Node>(current_token().value);
            node->add_child(op_node);
            advance();
            auto right_child = parse_C();
            node->add_child(right_child);
        }
        return node;
    }

    std::shared_ptr<Node> parse_C() {
        auto node = std::make_shared<Node>("C");
        if (current_token().type == "Идентификатор" || current_token().type == "Логическая константа") {
            node->add_child(std::make_shared<Node>(current_token().value));
            advance();
        }
        else if (current_token().value == "not") {
            auto not_node = std::make_shared<Node>(current_token().value);
            node->add_child(not_node);
            advance();
            auto child = parse_C();
            node->add_child(child);
        }
        else if (current_token().value == "(") {
            advance();
            auto child = parse_A();
            node->add_child(child);
            expect("Закрывающая скобка");
        }
        else {
            throw std::runtime_error("Неожиданный токен: " + current_token().value);
        }
        return node;
    }

public:
    SyntaxAnalyzer(const std::vector<Token>& tokens) : tokens(tokens), current_index(0) {}

    std::shared_ptr<Node> analyze() {
        return parse_E();
    }
};


// Структура для триад
struct Triad {
    int number;         // Номер триады
    std::string op;     // Оператор
    std::string arg1;   // Первый аргумент
    std::string arg2;   // Второй аргумент (может быть номером триады)
    std::string result; // Результат
};

// Генерация исходных триад
void generate_triads(const std::vector<Token>& tokens, std::vector<Triad>& triads, int& triad_counter) {
    std::stack<std::string> operands;
    std::stack<std::string> operators;
    std::map<std::string, int> last_triad_ref; // Хранение ссылки на последнюю триаду для каждой переменной
    int temp_var_counter = 1;

    auto get_temp_var = [&]() {
        return "^" + std::to_string(temp_var_counter++);
        };

    for (const auto& token : tokens) {
        if (token.type == "Идентификатор" || token.type == "Логическая константа") {
            operands.push(token.value);
        }
        else if (token.type == "Логический оператор" || token.value == ":=") {
            operators.push(token.value);
        }
        else if (token.type == "Закрывающая скобка") {
            while (!operators.empty() && operators.top() != "(") {
                std::string op = operators.top(); operators.pop();
                std::string right = operands.top(); operands.pop();
                std::string left = operands.top(); operands.pop();

                // Установка номера триады вместо значения, если известно
                std::string arg1 = left;
                if (last_triad_ref.count(left)) arg1 = std::to_string(last_triad_ref[left]);

                std::string arg2 = right;
                if (last_triad_ref.count(right)) arg2 = std::to_string(last_triad_ref[right]);

                std::string temp = get_temp_var();
                triads.push_back({ triad_counter++, op, arg1, arg2, temp });
                operands.push(temp);
                last_triad_ref[temp] = triad_counter - 1; // Обновляем ссылку
            }
            if (!operators.empty() && operators.top() == "(") {
                operators.pop();
            }
        }
        else if (token.type == "Открывающая скобка") {
            operators.push(token.value);
        }
    }

    while (!operators.empty()) {
        std::string op = operators.top(); operators.pop();
        if (op == ":=") {
            std::string value = operands.top(); operands.pop();
            std::string target = operands.top(); operands.pop();

            // Установка номера триады для операнда, если известно
            if (last_triad_ref.count(value)) value = std::to_string(last_triad_ref[value]);

            triads.push_back({ triad_counter++, op, target, value, "" });
            last_triad_ref[target] = triad_counter - 1; // Обновляем ссылку
        }
        else {
            std::string right = operands.top(); operands.pop();
            std::string left = operands.top(); operands.pop();

            // Установка номера триады для операндов, если известно
            if (last_triad_ref.count(left)) left = std::to_string(last_triad_ref[left]);
            if (last_triad_ref.count(right)) right = std::to_string(last_triad_ref[right]);

            std::string temp = get_temp_var();
            triads.push_back({ triad_counter++, op, left, right, temp });
            operands.push(temp);
            last_triad_ref[temp] = triad_counter - 1; // Обновляем ссылку
        }
    }
}

// Оптимизация триад
void optimize_triads_folding(const std::vector<Token>& tokens, std::vector<Triad>& triads,
    std::map<std::tuple<std::string, std::string, std::string>, std::string>& temp_vars,
    int& temp_var_counter, int& triad_counter) {
    std::stack<std::string> operands;
    std::stack<std::string> operators;
    std::map<std::string, std::string> updated_vars; // Карта обновлений переменных

    auto get_temp_var = [&]() {
        return "^" + std::to_string(temp_var_counter++);
        };

    auto add_triad = [&](const std::string& op, const std::string& left, const std::string& right) -> std::string {
        auto expression = std::make_tuple(op, left, right);

        // Если выражение уже вычислено
        if (temp_vars.find(expression) != temp_vars.end()) {
            return temp_vars[expression];
        }

        // Генерация новой триады
        std::string temp = get_temp_var();
        temp_vars[expression] = temp;
        triads.push_back({ triad_counter++, op, left, right, temp });
        return temp;
        };

    for (const auto& token : tokens) {
        if (token.type == "Идентификатор" || token.type == "Логическая константа") {
            // Если переменная была обновлена, использовать её новое значение
            if (updated_vars.count(token.value)) {
                operands.push(updated_vars[token.value]);
            }
            else {
                operands.push(token.value);
            }
        }
        else if (token.type == "Логический оператор" || token.value == ":=") {
            operators.push(token.value);
        }
        else if (token.type == "Закрывающая скобка") {
            while (!operators.empty() && operators.top() != "(") {
                std::string op = operators.top(); operators.pop();
                std::string right = operands.top(); operands.pop();
                std::string left = operands.top(); operands.pop();

                std::string temp = add_triad(op, left, right);
                operands.push(temp);
            }
            if (!operators.empty() && operators.top() == "(") {
                operators.pop();
            }
        }
        else if (token.type == "Открывающая скобка") {
            operators.push(token.value);
        }
    }

    while (!operators.empty()) {
        std::string op = operators.top(); operators.pop();
        if (op == ":=") {
            std::string value = operands.top(); operands.pop();
            std::string target = operands.top(); operands.pop();

            // Добавляем новую триаду для присваивания
            triads.push_back({ triad_counter++, op, target, value, "" });

            // Обновляем карту обновлений
            updated_vars[target] = value;

            // Удаляем старые триады, связанные с обновлённой переменной
            auto it = temp_vars.begin();
            while (it != temp_vars.end()) {
                if (std::get<0>(it->first) == target || std::get<1>(it->first) == target) {
                    it = temp_vars.erase(it);
                }
                else {
                    ++it;
                }
            }
        }
        else {
            std::string right = operands.top(); operands.pop();
            std::string left = operands.top(); operands.pop();

            std::string temp = add_triad(op, left, right);
            operands.push(temp);
        }
    }
}

// Вывод триад
void print_triads(const std::string& title, const std::vector<Triad>& triads) {
    std::cout << title << ":\n";
    for (const auto& triad : triads) {
        std::cout << triad.number << ". " << triad.op
            << " [" << triad.arg1 << ", " << triad.arg2 << "] " << std::endl;
    }
}


int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "RUS");

    if (argc < 2) {
        std::cerr << "Ошибка: Не указан входной файл." << std::endl;
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Ошибка: Не удалось открыть файл." << std::endl;
        return 1;
    }
    //std::ifstream file("C:/Users/andpm/Semestr7/ИСИС/Генерация и оптимизация/7.txt");

    std::vector<Token> tokens;
    std::string line;
    int line_number = 1;

    try {
        while (std::getline(file, line)) {
            if (line.empty()) {
                line_number++;
                continue;
            }
            auto line_tokens = tokenize(line, line_number);
            tokens.insert(tokens.end(), line_tokens.begin(), line_tokens.end());
            line_number++;
        }

        // Вывод токенов
        for (const auto& token : tokens) {
            std::cout << token.line_number << "," << token.type << "," << token.value << std::endl;
        }

        // Синтаксический анализ
        SyntaxAnalyzer analyzer(tokens);
        auto syntax_tree = analyzer.analyze();


        // Вывод дерева
        std::cout << "TREE:" << std::endl;
        syntax_tree->print();

        // Разделение выражений
        auto expressions = split_expressions(tokens);

        // Генерация и оптимизация триад
        std::vector<Triad> original_triads, optimized_triads;
        std::map<std::tuple<std::string, std::string, std::string>, std::string> temp_vars;
        int temp_var_counter = 1, triad_counter = 1;

        int original_triad_counter = 1;   // Счетчик для исходных триад
        int optimized_triad_counter = 1; // Счетчик для оптимизированных триад

        for (const auto& expr_tokens : expressions) {
            generate_triads(expr_tokens, original_triads, original_triad_counter); // Используем отдельный счетчик
            optimize_triads_folding(expr_tokens, optimized_triads, temp_vars, temp_var_counter, optimized_triad_counter);
        }

        // Вывод результатов
        print_triads("Исходные триады", original_triads);
        print_triads("\nТриады после оптимизации", optimized_triads);

    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}