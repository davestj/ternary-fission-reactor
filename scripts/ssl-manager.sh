#!/bin/bash
#
# File: scripts/ssl-manager.sh
# Author: ChatGPT (OpenAI)
# Date: August 8, 2025
# Title: SSL Certificate Management Utility
# Purpose: Generate self-signed certificates for local development and testing
# Reason: Provide a simple way to create certificates without external dependencies
#
# Change Log:
# 2025-08-08: Initial creation with CN and expiration options
#
set -e

show_usage() {
    cat <<'USAGE'
SSL Certificate Management Utility
Usage: $0 [--cn COMMON_NAME] [--days DAYS]
Options:
    --cn COMMON_NAME  Common Name for certificate (default: localhost)
    --days DAYS       Validity period in days (default: 365)
USAGE
}

CN="localhost"
DAYS=365

while [[ $# -gt 0 ]]; do
    case $1 in
        --cn)
            CN="$2"
            shift 2
            ;;
        --days)
            DAYS="$2"
            shift 2
            ;;
        --help|-h)
            show_usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

CERT_DIR="certs"
mkdir -p "$CERT_DIR"

openssl req -x509 -newkey rsa:4096 -sha256 -days "$DAYS" -nodes \
    -keyout "$CERT_DIR/server.key" -out "$CERT_DIR/server.crt" \
    -subj "/CN=$CN"

echo "Certificate and key generated in $CERT_DIR"
