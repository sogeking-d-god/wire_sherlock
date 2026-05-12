FROM python:3.12-slim

# Install system dependencies for C engine and networking
RUN apt-get update && apt-get install -y \
    gcc \
    make \
    libpcap-dev \
    libcjson-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# COPY requirements from the root (since context is .)
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# COPY everything from the root into /app
COPY . .

# Build the C engine - Path is relative to /app
# Ensure 'api/engine' exists in your root or backend folder
RUN cd api/engine && make

EXPOSE 8000

# Run the FastAPI server
# Using -m backend.main assuming main.py is inside the backend folder
CMD ["python", "-m", "backend.main"]