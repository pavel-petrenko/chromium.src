<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output method="html"/>
  <xsl:template match="RESULT">
    <html>
      <script>
        if (window.testRunner)
        testRunner.dumpAsText();
      </script>
      PASSED
    </html>
  </xsl:template>
</xsl:stylesheet>
