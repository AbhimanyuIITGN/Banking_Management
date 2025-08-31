#!/bin/bash

# PDF Generation Script for Technical Report
# This script converts the markdown report to a professional PDF

echo "🎯 Generating PDF Report for ATM Banking System..."

# Check if pandoc is installed
if ! command -v pandoc &> /dev/null; then
    echo "❌ Pandoc is not installed. Installing via Homebrew..."
    if command -v brew &> /dev/null; then
        brew install pandoc
        brew install --cask basictex  # For LaTeX support
    else
        echo "❌ Please install Homebrew first: https://brew.sh/"
        echo "Then run: brew install pandoc"
        exit 1
    fi
fi

# Check if wkhtmltopdf is available (alternative to LaTeX)
if command -v wkhtmltopdf &> /dev/null; then
    echo "✅ Using wkhtmltopdf for PDF generation..."
    
    # Generate PDF with wkhtmltopdf (better formatting)
    pandoc TECHNICAL_REPORT.md \
        --from markdown \
        --to html5 \
        --css style.css \
        --self-contained \
        --output temp_report.html
    
    wkhtmltopdf \
        --page-size A4 \
        --margin-top 0.75in \
        --margin-right 0.75in \
        --margin-bottom 0.75in \
        --margin-left 0.75in \
        --encoding UTF-8 \
        --print-media-type \
        temp_report.html \
        "ATM_Banking_System_Technical_Report.pdf"
    
    rm temp_report.html
    
elif command -v pdflatex &> /dev/null; then
    echo "✅ Using LaTeX for PDF generation..."
    
    # Generate PDF with LaTeX (professional formatting)
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
        
else
    echo "⚠️  LaTeX not found. Generating basic PDF..."
    
    # Basic PDF generation
    pandoc TECHNICAL_REPORT.md \
        --from markdown \
        --to pdf \
        --variable geometry:margin=1in \
        --toc \
        --number-sections \
        --output "ATM_Banking_System_Technical_Report.pdf"
fi

# Check if PDF was generated successfully
if [ -f "ATM_Banking_System_Technical_Report.pdf" ]; then
    echo "🎉 PDF Report Generated Successfully!"
    echo "📄 File: ATM_Banking_System_Technical_Report.pdf"
    echo "📊 Size: $(du -h ATM_Banking_System_Technical_Report.pdf | cut -f1)"
    echo ""
    echo "🔗 To open the PDF:"
    echo "   macOS: open ATM_Banking_System_Technical_Report.pdf"
    echo "   Linux: xdg-open ATM_Banking_System_Technical_Report.pdf"
    echo "   Windows: start ATM_Banking_System_Technical_Report.pdf"
    echo ""
    echo "📧 Ready for sharing with recruiters and interviewers!"
else
    echo "❌ PDF generation failed. Please check the error messages above."
    echo ""
    echo "🔧 Manual conversion options:"
    echo "1. Use online converter: https://pandoc.org/try/"
    echo "2. Use VS Code with Markdown PDF extension"
    echo "3. Use GitHub to render and print to PDF"
    echo "4. Use Typora or other markdown editors with PDF export"
fi

echo ""
echo "📋 Report Contents:"
echo "   • Executive Summary"
echo "   • Core CS Subjects (8 major areas)"
echo "   • Technical Implementation Details"
echo "   • System Architecture Diagrams"
echo "   • Performance Analysis"
echo "   • Interview Preparation Guide"
echo "   • Code Examples and Algorithms"
echo "   • Security Implementation"
echo "   • Scalability Analysis"
echo "   • Career Positioning Guide"
