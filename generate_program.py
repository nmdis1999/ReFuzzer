import os

import ollama


def chat_with_ollama(prompt, model="llama3.2"):
    try:
        response = ollama.chat(
            model=model,
            messages=[
                {"role": "user", "content": prompt},
            ],
        )
        return response["message"]["content"]
    except Exception as e:
        return f"An error occurred: {str(e)}"


def truncate_code_blocks(content):
    lines = content.split("\n")
    if (
        len(lines) >= 2
        and lines[0].strip().startswith("```")
        and lines[-1].strip().startswith("```")
    ):
        return "\n".join(lines[1:-1])
    return content


def save_to_file(response, filename):
    directory = "cpp_files"

    if not os.path.exists(directory):
        os.makedirs(directory)
        print(f"Generated files will be stored in {directory}")

    filename += ".cpp"
    file_path = os.path.join(directory, filename)

    # Truncate code blocks before saving
    truncated_response = truncate_code_blocks(response)

    with open(file_path, "w") as file:
        file.write(truncated_response)

    print(f"File {filename} is created\n")


if __name__ == "__main__":
    with open("list.txt", "r") as file:
        for line in file:
            item = line.strip()
            prompt = "Generate C++ code for " + item + " and only show me code"
            response = chat_with_ollama(prompt)
            save_to_file(response, item)
