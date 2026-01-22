import subprocess
import tkinter as tk
from tkinter import filedialog, messagebox, scrolledtext, ttk

def run_syntax_analyzer(file_path):
    """Вызов C++ программы для синтаксического анализа."""
    try:
        result = subprocess.run(
            ["C:/Users/andpm/Semestr7/ИСИС/Генерация и оптимизация/ConsoleApplication1/x64/Debug/ConsoleApplication1.exe", file_path],
            text=True,
            capture_output=True,
            check=True
        )
        return result.stdout
    except subprocess.CalledProcessError as e:
        messagebox.showerror("Ошибка", e.stderr)
        return ""

def parse_output(output):
    """Парсинг вывода C++ программы."""
    lexemes_table = []
    tree_lines = []
    triads_original = []
    triads_optimized = []

    current_section = None

    for line in output.strip().split("\n"):
        
        if not line:
            continue  # Пропускаем пустые строки

        # Определяем текущую секцию
        if line.startswith("TREE:"):
            current_section = "tree"
            continue
        elif line.startswith("Исходные триады:"):
            current_section = "triads_original"
            continue
        elif line.startswith("Триады после оптимизации:"):
            current_section = "triads_optimized"
            continue

        # Обработка строк в зависимости от текущей секции
        if current_section == "tree":
            tree_lines.append(line)
        elif current_section == "triads_original":
            triads_original.append(line)
        elif current_section == "triads_optimized":
            triads_optimized.append(line)
        else:
            parts = line.split(",", 2)
            if len(parts) == 3:
                line_number, token_type, value = parts
                lexemes_table.append({
                    '№': line_number.strip(),
                    'Тип': token_type.strip(),
                    'Значение': value.strip()
                })

    tree = "\n".join(tree_lines)
    return lexemes_table, tree, triads_original, triads_optimized

def open_file_and_analyze():
    """Выбор файла и анализ."""
    file_path = filedialog.askopenfilename(title="Выберите файл для анализа", filetypes=[("Text Files", "*.txt")])
    if file_path:
        try:
            with open(file_path, 'r', encoding='utf-8') as file:
                file_content = file.read()
                file_content_text.delete(1.0, tk.END)
                file_content_text.insert(tk.END, file_content)

            output = run_syntax_analyzer(file_path)
            if output:
                lexemes_table, tree, triads_original, triads_optimized = parse_output(output)
                display_lexemes_in_table(lexemes_table)
                display_parse_tree(tree)
                display_triads(triads_original_text, triads_original)
                display_triads(triads_cleaned_text, triads_optimized)
        except Exception as e:
            messagebox.showerror("Ошибка", f"Не удалось открыть файл: {e}")
    else:
        messagebox.showwarning("Файл не выбран", "Пожалуйста, выберите файл для анализа.")

def display_lexemes_in_table(lexemes_table):
    """Отображение таблицы лексем в Treeview."""
    for item in result_table.get_children():
        result_table.delete(item)

    for lexeme in lexemes_table:
        result_table.insert("", "end", values=(lexeme['№'], lexeme['Тип'], lexeme['Значение']))

def display_parse_tree(tree):
    """Отображение дерева синтаксического разбора."""
    parse_tree_text.delete(1.0, tk.END)
    parse_tree_text.insert(tk.END, tree)

def display_triads(text_widget, triads):
    """Отображение триад в текстовом поле."""
    text_widget.delete(1.0, tk.END)
    text_widget.insert(tk.END, "\n".join(triads))

# Интерфейс tkinter
root = tk.Tk()
root.title("Генерация и оптимизация")
root.geometry("1200x700")

# Основной интерфейс с вкладками
notebook = ttk.Notebook(root)
notebook.pack(fill="both", expand=True)

# Вкладка синтаксического анализа
syntax_frame = tk.Frame(notebook)
notebook.add(syntax_frame, text="Лексический и Синтаксический анализ")

# Вкладка триад
triads_frame = tk.Frame(notebook)
notebook.add(triads_frame, text="Триады")

# Левый фрейм для выбора файла и отображения его содержимого
left_frame = tk.Frame(syntax_frame)
left_frame.pack(side="left", fill="both", expand=True, padx=10, pady=10)

# Кнопка для выбора файла
select_file_button = tk.Button(left_frame, text="Выбрать файл для анализа", command=open_file_and_analyze)
select_file_button.pack(pady=5)

# Поле для отображения содержимого файла
file_content_label = tk.Label(left_frame, text="Содержимое файла:")
file_content_label.pack(anchor="w")
file_content_text = scrolledtext.ScrolledText(left_frame, wrap=tk.WORD, width=40, height=20)
file_content_text.pack(pady=5, fill="both", expand=True)

# Средний фрейм для таблицы лексем
middle_frame = tk.Frame(syntax_frame)
middle_frame.pack(side="left", fill="both", expand=True, padx=10, pady=10)

# Таблица для отображения результата анализа
result_label = tk.Label(middle_frame, text="Результаты лексического анализа:")
result_label.pack(anchor="w")
columns = ("№", "Тип", "Значение")
result_table = ttk.Treeview(middle_frame, columns=columns, show="headings")
result_table.heading("№", text="№")
result_table.heading("Тип", text="Тип")
result_table.heading("Значение", text="Значение")
result_table.column("№", width=50, anchor="center")
result_table.column("Тип", width=150, anchor="center")
result_table.column("Значение", width=150, anchor="center")

# Прокрутка для таблицы
scrollbar = ttk.Scrollbar(middle_frame, orient="vertical", command=result_table.yview)
result_table.configure(yscroll=scrollbar.set)
scrollbar.pack(side="right", fill="y")
result_table.pack(pady=5, fill="both", expand=True)

# Правый фрейм для дерева синтаксического разбора
right_frame = tk.Frame(syntax_frame)
right_frame.pack(side="right", fill="both", expand=True, padx=10, pady=10)

# Поле для отображения дерева разбора
parse_tree_label = tk.Label(right_frame, text="Дерево синтаксического разбора:")
parse_tree_label.pack(anchor="w")
parse_tree_text = scrolledtext.ScrolledText(right_frame, wrap=tk.WORD, width=40, height=20)
parse_tree_text.pack(pady=5, fill="both", expand=True)

# Поля для отображения триад
triads_original_frame = tk.Frame(triads_frame, width=300)  # Уменьшение ширины фрейма
triads_original_frame.pack(side="left", fill="y", padx=10, pady=10)  # fill="y" для фиксированной ширины

triads_original_label = tk.Label(triads_original_frame, text="Исходные триады:")
triads_original_label.pack(anchor="w")

triads_original_text = scrolledtext.ScrolledText(triads_original_frame, wrap=tk.WORD, width=40, height=50)  # Уменьшение ширины текстового поля
triads_original_text.pack(padx=5, pady=5)

triads_cleaned_frame = tk.Frame(triads_frame, width=300)  # Уменьшение ширины фрейма
triads_cleaned_frame.pack(side="left", fill="y", padx=10, pady=10)

triads_cleaned_label = tk.Label(triads_cleaned_frame, text="Триады после оптимизации:")
triads_cleaned_label.pack(anchor="w")

triads_cleaned_text = scrolledtext.ScrolledText(triads_cleaned_frame, wrap=tk.WORD, width=40, height=50)  # Уменьшение ширины текстового поля
triads_cleaned_text.pack(padx=5, pady=5)


# Запуск интерфейса

root.mainloop()
