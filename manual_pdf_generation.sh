#!/bin/bash

# Manual PDF Generation (after pandoc installation)
echo "🎯 Generating Professional PDF Report..."

# Update PATH to include LaTeX
eval "$(/usr/libexec/path_helper)"

# Generate PDF with professional formatting
pandoc TECHNICAL_REPORT.md \
    --from markdown \
    --to pdf \
    --pdf-engine=pdflatex \
    --variable geometry:margin=1in \
    --variable fontsize=11pt \
    --variable documentclass=article \
    --variable colorlinks=true \
    --variable linkcolor=blue \
    --variable urlcolor=blue \
    --variable toccolor=black \
    --toc \
    --toc-depth=3 \
    --number-sections \
    --highlight-style=github \
    --output "ATM_Banking_System_Technical_Report.pdf"

if [ -f "ATM_Banking_System_Technical_Report.pdf" ]; then
    echo "🎉 PDF Generated Successfully!"
    echo "📄 File: ATM_Banking_System_Technical_Report.pdf"
    echo "📊 Size: $(du -h ATM_Banking_System_Technical_Report.pdf | cut -f1)"
    open ATM_Banking_System_Technical_Report.pdf
else
    echo "❌ PDF generation failed. Using basic conversion..."
    
    # Fallback: Basic PDF without LaTeX
    pandoc TECHNICAL_REPORT.md \
        --from markdown \
        --to pdf \
        --variable geometry:margin=1in \
        --toc \
        --number-sections \
        --output "ATM_Banking_System_Technical_Report_Basic.pdf"
    
    if [ -f "ATM_Banking_System_Technical_Report_Basic.pdf" ]; then
        echo "✅ Basic PDF generated: ATM_Banking_System_Technical_Report_Basic.pdf"
        open ATM_Banking_System_Technical_Report_Basic.pdf
    fi
fi
