FROM python:3.12-slim

# Install system dependencies for C engine and networking
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    make \
    libpcap-dev \
    libcjson-dev \
    binutils \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# COPY requirements from the root (since context is .)
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY backend/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

EXPOSE 8000

ENTRYPOINT ["/entrypoint.sh"]
CMD ["python", "-m", "backend.main"]