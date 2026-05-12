FROM node:20-slim

WORKDIR /app

# Copy package files first for better caching
COPY package*.json ./

# Install dependencies including peer dependencies
RUN npm install

# Copy the rest of the frontend source code
COPY . .

# Vite default port
EXPOSE 5173

# IMPORTANT: --host 0.0.0.0 is mandatory to access the container from Windows
# We use 'npm run dev' which triggers the vite command from your package.json
CMD ["npm", "run", "dev", "--", "--host", "0.0.0.0"]