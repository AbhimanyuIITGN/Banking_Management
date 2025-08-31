#!/bin/bash

# Generate HTML version for browser PDF conversion
echo "🎯 Generating HTML version for PDF conversion..."

# Check if pandoc is available (might be installed now)
if command -v pandoc &> /dev/null; then
    echo "✅ Pandoc found! Generating HTML..."
    
    # Generate professional HTML
    pandoc TECHNICAL_REPORT.md \
        --from markdown \
        --to html5 \
        --css style.css \
        --self-contained \
        --toc \
        --toc-depth=3 \
        --number-sections \
        --highlight-style=github \
        --metadata title="ATM Banking System - Technical Report" \
        --output "ATM_Technical_Report.html"
    
    if [ -f "ATM_Technical_Report.html" ]; then
        echo "🎉 HTML Report Generated Successfully!"
        echo "📄 File: ATM_Technical_Report.html"
        echo ""
        echo "🖨️  To convert to PDF:"
        echo "1. Open ATM_Technical_Report.html in your browser"
        echo "2. Press Cmd+P (Mac) or Ctrl+P (Windows/Linux)"
        echo "3. Select 'Save as PDF' as destination"
        echo "4. Choose 'More settings' and enable 'Background graphics'"
        echo "5. Save as 'ATM_Banking_System_Technical_Report.pdf'"
        echo ""
        echo "🚀 Opening HTML file in browser..."
        open ATM_Technical_Report.html
    else
        echo "❌ HTML generation failed"
    fi
else
    echo "⏳ Pandoc not yet available. Creating simple HTML..."
    
    # Create basic HTML wrapper
    cat > ATM_Technical_Report_Simple.html << 'EOF'
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>ATM Banking System - Technical Report</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 800px; margin: 0 auto; padding: 20px; }
        h1 { color: #2c3e50; border-bottom: 2px solid #3498db; }
        h2 { color: #34495e; border-bottom: 1px solid #ecf0f1; }
        pre { background: #f8f9fa; padding: 15px; border-radius: 5px; overflow-x: auto; }
        code { background: #f1f3f4; padding: 2px 4px; border-radius: 3px; }
        table { border-collapse: collapse; width: 100%; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #3498db; color: white; }
    </style>
</head>
<body>
EOF
    
    # Convert markdown to basic HTML (manual conversion)
    echo "<h1>ATM Banking System - Technical Report</h1>" >> ATM_Technical_Report_Simple.html
    echo "<p><strong>Status:</strong> Pandoc installation in progress...</p>" >> ATM_Technical_Report_Simple.html
    echo "<p><strong>File:</strong> TECHNICAL_REPORT.md contains the complete report</p>" >> ATM_Technical_Report_Simple.html
    echo "<p><strong>Instructions:</strong> Use online converter or wait for pandoc installation</p>" >> ATM_Technical_Report_Simple.html
    echo "</body></html>" >> ATM_Technical_Report_Simple.html
    
    echo "📄 Basic HTML created: ATM_Technical_Report_Simple.html"
    open ATM_Technical_Report_Simple.html
fi

echo ""
echo "📋 Available Files:"
ls -la *.md *.html *.pdf 2>/dev/null || echo "   TECHNICAL_REPORT.md (source file)"
echo ""
echo "🎯 Next Steps:"
echo "1. Wait for pandoc installation to complete"
echo "2. Run: ./manual_pdf_generation.sh"
echo "3. Or use online converter with TECHNICAL_REPORT.md"
echo "4. Or use browser print-to-PDF with HTML file"
