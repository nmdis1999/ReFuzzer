FROM ollama/ollama:latest
ENV OLLAMA_HOST=0.0.0.0
RUN apt-get update && apt-get install -y clang nlohmann-json3-dev curl cmake python3 python3-pip git && rm -rf /var/lib/apt/lists/*
RUN git clone https://github.com/nmdis1999/ReFuzzer.git /app/ReFuzzer
EXPOSE 11434
ENTRYPOINT ["sh", "-c", "nohup ollama serve > /dev/null 2>&1 & sleep 5 && nohup ollama pull starcoder > /dev/null 2>&1 & wait"]
