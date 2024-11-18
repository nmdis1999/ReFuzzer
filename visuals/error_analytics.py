import os
import re
from collections import defaultdict
from typing import Dict, Set, Tuple

import matplotlib.pyplot as plt
from fpdf import FPDF
from langchain_ollama import OllamaLLM

ERROR_CATEGORIES = {
    "SYNTAX": "Syntax Error",
    "TYPE": "Type Error", 
    "LINKER": "Linker Error",
    "DECLARATION": "Declaration Error",
    "PREPROCESSING": "Preprocessing Error",
    "MEMORY": "Memory Error",
    "INITIALIZATION": "Initialization Error",
    "PARAMETER": "Parameter Error",
    "SCOPE": "Scope Error",
    "STANDARD": "Standard Error",
    "HEADER": "Header Error",
    "NAMING": "Naming Error",
    "OPERATOR": "Operator Error",
    "SIZE": "Size Error",
    "OVERFLOW": "Overflow Error"
}

class ErrorAnalyzer:
    def __init__(self, model_name: str = "llama3.2", temperature: float = 0.1):
        """
        Initialize the error analyzer with optimized LLM settings.
        
        Args:
            model_name: The name of the Ollama model to use
            temperature: Controls randomness in the output (0.0-1.0)
                        Lower values = more deterministic
                        Higher values = more creative/random
        """
        self.ollama = OllamaLLM(
            model=model_name,
            temperature=temperature,
        )
    
    def get_error_category(self, error_text: str) -> str:
        """Categorize an error message using the LLM with strict formatting."""
        prompt = f"""You are a compiler error analysis expert. Given a gcc compiler error/warning, 
        classify it into EXACTLY ONE of these categories by responding with just the category name:

        {'\n'.join(ERROR_CATEGORIES.keys())}

        Rules:
        - Respond with EXACTLY ONE word from the above list
        - Do not add any explanation or additional text
        - If unsure, prefer more specific categories over general ones

        Error text to analyze: {error_text}
        
        Category:"""
        
        try:
            response = self.ollama(prompt).strip().upper()
            if response not in ERROR_CATEGORIES:
                print(f"Warning: Unexpected category '{response}' received")
                return "Other Error"
            return ERROR_CATEGORIES[response]
        except Exception as e:
            print(f"Error in categorization: {e}")
            return "Uncategorized Error"

    @staticmethod
    def get_error_context(content: str, error_match: re.Match, context_lines: int = 3) -> str:
        """Extract error message with surrounding context."""
        try:
            error_start = error_match.start()
            error_end = error_match.end()
            
            # Find context boundaries
            line_start = max(0, content.rfind("\n", 0, error_start) + 1)
            
            # Get previous context lines
            for _ in range(context_lines):
                prev_start = content.rfind("\n", 0, line_start - 1)
                if prev_start == -1:
                    break
                line_start = prev_start + 1
                
            # Get following context lines
            line_end = error_end
            for _ in range(context_lines):
                next_end = content.find("\n", line_end + 1)
                if next_end == -1:
                    line_end = len(content)
                    break
                line_end = next_end
                
            # Format context with highlighting
            context = content[line_start:line_end].strip()
            return "\n".join(
                f">>> {line}" if error_start <= content.find(line, line_start) <= error_end
                else f"    {line}"
                for line in context.split("\n")
            )
        except Exception as e:
            print(f"Error getting context: {e}")
            return "Context extraction failed"

    def process_error_block(self, block: str, file: str) -> Tuple[Dict[str, Set[str]], Dict[str, str]]:
        """Process a single error block and return categorized errors and examples."""
        error_files = defaultdict(set)
        error_examples = defaultdict(str)
        
        patterns = {
            "error": r"(?:error:|fatal error:)\s*(.+?)(?=\n|(?:error:|warning:|fatal error:)|$)",
            "warning": r"warning:\s*(.+?)(?=\n|(?:error:|warning:|fatal error:)|$)"
        }
        
        for pattern_type, pattern in patterns.items():
            matches = re.finditer(pattern, block, re.DOTALL | re.MULTILINE)
            
            for match in matches:
                try:
                    if match and match.group(1):
                        message = match.group(1).strip()
                        if not message:
                            continue
                            
                        category = self.get_error_category(message)
                        error_files[category].add(file)
                        
                        if not error_examples[category]:
                            context = self.get_error_context(block, match)
                            error_examples[category] = (
                                f"File: {file}\n"
                                f"Context:\n{context}\n"
                                f"{'Error' if pattern_type == 'error' else 'Warning'} Message: {message}"
                            )
                except Exception as e:
                    print(f"Error processing match in {file}: {e}")
                    continue
                    
        return error_files, error_examples

    def analyze_logs(self, log_dir: str, max_files: int = 10) -> Tuple[Dict[str, int], Dict[str, str]]:
        """Analyze log files and return error statistics and examples."""
        try:
            log_files = [f for f in os.listdir(log_dir) if f.endswith(".log")][:max_files]
            total_files = len(log_files)
            print(f"Processing {total_files} files...")
            
            all_error_files = defaultdict(set)
            all_error_examples = defaultdict(str)
            
            for i, file in enumerate(log_files, 1):
                try:
                    print(f"Processing file {i}/{total_files}: {file}")
                    
                    with open(os.path.join(log_dir, file), "r", encoding='utf-8') as f:
                        content = f.read()
                        error_blocks = [block.strip() for block in re.split(r"-{40,}", content) if block.strip()]
                        
                        for block in error_blocks:
                            try:
                                error_files, error_examples = self.process_error_block(block, file)
                                
                                for category, files in error_files.items():
                                    all_error_files[category].update(files)
                                for category, example in error_examples.items():
                                    if not all_error_examples[category]:
                                        all_error_examples[category] = example
                            except Exception as e:
                                print(f"Error processing block in file {file}: {str(e)}")
                                continue
                                
                except Exception as e:
                    print(f"Error processing file {file}: {str(e)}")
                    continue
                    
            return {k: len(v) for k, v in all_error_files.items()}, all_error_examples
            
        except Exception as e:
            print(f"Error analyzing logs: {str(e)}")
            return {}, {}

class ReportGenerator:
    @staticmethod
    def create_reports(error_counts: Dict[str, int], error_examples: Dict[str, str], output_dir: str = "analysis"):
        """Generate PDF reports with error statistics and examples."""
        os.makedirs(output_dir, exist_ok=True)
        
        # Create statistics chart
        plt.figure(figsize=(10, 6))
        data = sorted(error_counts.items(), key=lambda x: x[1], reverse=True)
        categories, counts = zip(*data) if data else ([], [])
        
        table = plt.table(
            cellText=[[cat, str(count)] for cat, count in zip(categories, counts)],
            colLabels=["Error Type", "File Count"],
            loc="center",
            cellLoc="left",
            colWidths=[0.6, 0.4]
        )
        
        # Format table
        table.auto_set_font_size(False)
        table.set_fontsize(9)
        table.scale(1.2, 1.5)
        
        # Style header
        cells = table.get_celld()
        for j in range(2):
            cells[(0, j)].set_text_props(weight="bold", color="white")
            cells[(0, j)].set_facecolor("#4472C4")
        
        plt.title("Error Type Distribution", pad=20)
        plt.axis("off")
        
        # Save charts and examples
        plt.savefig(
            os.path.join(output_dir, "error_statistics.pdf"),
            bbox_inches="tight",
            dpi=300
        )
        plt.close()
        
        # Generate examples PDF
        pdf = FPDF()
        pdf.add_page()
        pdf.set_font("Arial", "B", 14)
        pdf.cell(0, 10, "Error Examples", ln=True)
        pdf.ln(5)
        
        for category in categories:
            if category in error_examples:
                pdf.set_font("Arial", "B", 11)
                pdf.cell(0, 10, f"{category}", ln=True)
                pdf.set_font("Arial", "", 10)
                pdf.multi_cell(0, 5, error_examples[category])
                pdf.ln(5)
                
        pdf.output(os.path.join(output_dir, "error_examples.pdf"))

def main():
    import time
    start_time = time.time()
    
    log_directory = os.path.expanduser("~/research/ollama/generate_c_code/log")
    print(f"Starting analysis of {log_directory}")
    
    analyzer = ErrorAnalyzer(model_name="llama3.2")
    error_counts, error_examples = analyzer.analyze_logs(log_directory)
    
    if error_counts and error_examples:
        report_gen = ReportGenerator()
        report_gen.create_reports(error_counts, error_examples)
        
        elapsed_time = time.time() - start_time
        print(f"\nAnalysis completed in {elapsed_time/60:.2f} minutes")
        print("Results saved in ./analysis directory")
    else:
        print("\nNo errors were processed successfully")

if __name__ == "__main__":
    main()
