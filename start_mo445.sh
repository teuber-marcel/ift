#!/bin/bash

# MO445 Environment Startup Script
echo "🚀 Starting MO445 Environment..."

# Activate conda environment
source ~/miniconda3/bin/activate mo445

# Check if environment is activated
if [[ "$CONDA_DEFAULT_ENV" == "mo445" ]]; then
    echo "✅ MO445 environment activated successfully"
    echo "📚 Starting Jupyter Notebook..."
    echo "🌐 Jupyter will open in your browser at: http://localhost:8888"
    echo "📁 Navigate to the 'notebooks/' directory to access the course materials"
    echo ""
    echo "Press Ctrl+C to stop Jupyter when you're done"
    echo "================================================"
    
    # Start Jupyter notebook
    jupyter notebook
else
    echo "❌ Failed to activate MO445 environment"
    echo "Please run: conda activate mo445"
    exit 1
fi
