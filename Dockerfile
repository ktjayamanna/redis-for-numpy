FROM ubuntu:focal/python-3.10

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install essential development tools
RUN apt-get update && apt-get install -y \
    gcc \
    g++ \
    make \
    gdb \
    valgrind \
    nano \
    curl \
    netcat-openbsd \
    tree \
    git \
    tcl \
    python3-pip \
    && pip3 install numpy \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /workspace

# Expose port 8080 for the web server
EXPOSE 8080

# Keep container running for development
CMD ["sleep", "infinity"]

