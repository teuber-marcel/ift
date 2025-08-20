#!/usr/bin/env python3
"""
Test script to verify MO445 environment setup
"""

import sys
import numpy as np
import matplotlib.pyplot as plt
from PIL import Image
import pandas as pd
import seaborn as sns

print("Testing MO445 Environment Setup...")
print("=" * 50)

# Test basic imports
print("✓ Basic Python packages imported successfully")

# Test PyIFT
try:
    import pyift
    print("✓ PyIFT imported successfully")
    
    # Test basic PyIFT functionality
    print(f"✓ PyIFT version: {pyift.__version__ if hasattr(pyift, '__version__') else 'Version info not available'}")
    
except ImportError as e:
    print(f"✗ PyIFT import failed: {e}")
    sys.exit(1)

# Test numpy
try:
    arr = np.random.rand(10, 10)
    print("✓ NumPy working correctly")
except Exception as e:
    print(f"✗ NumPy test failed: {e}")

# Test matplotlib
try:
    fig, ax = plt.subplots(1, 1, figsize=(6, 4))
    ax.plot([1, 2, 3, 4], [1, 4, 2, 3])
    plt.close(fig)
    print("✓ Matplotlib working correctly")
except Exception as e:
    print(f"✗ Matplotlib test failed: {e}")

# Test PIL
try:
    img = Image.new('RGB', (100, 100), color='red')
    print("✓ PIL (Pillow) working correctly")
except Exception as e:
    print(f"✗ PIL test failed: {e}")

# Test scikit-learn
try:
    from sklearn.cluster import KMeans
    kmeans = KMeans(n_clusters=3, random_state=42)
    print("✓ Scikit-learn working correctly")
except Exception as e:
    print(f"✗ Scikit-learn test failed: {e}")

# Test scikit-image
try:
    from skimage import filters
    from skimage import morphology
    print("✓ Scikit-image working correctly")
except Exception as e:
    print(f"✗ Scikit-image test failed: {e}")

# Test pandas
try:
    df = pd.DataFrame({'A': [1, 2, 3], 'B': [4, 5, 6]})
    print("✓ Pandas working correctly")
except Exception as e:
    print(f"✗ Pandas test failed: {e}")

# Test seaborn
try:
    sns.set_theme()
    print("✓ Seaborn working correctly")
except Exception as e:
    print(f"✗ Seaborn test failed: {e}")

print("\n" + "=" * 50)
print("🎉 MO445 Environment Setup Complete!")
print("\nTo start working with the notebooks:")
print("1. Activate the environment: conda activate mo445")
print("2. Start Jupyter: jupyter notebook")
print("3. Navigate to the notebooks/ directory")
print("\nAvailable notebooks:")
print("- ConnectedOperators.ipynb")
print("- MathMorphology.ipynb")
print("- Skeletonization.ipynb")
print("- Watershed.ipynb")
print("- Labeling.ipynb")
print("- Convolution.ipynb")
print("- segmentation_and_classification/ (directory with multiple notebooks)")
print("- pytorch_introduction/ (directory with PyTorch tutorials)")
print("\nNote: OpenCV has compatibility issues on macOS but core functionality works.")
print("Most notebooks should work without OpenCV.")
