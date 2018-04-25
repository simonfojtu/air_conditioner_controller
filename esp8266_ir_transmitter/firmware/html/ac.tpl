<html><head><title>AC control</title>
<link rel="stylesheet" type="text/css" href="style.css">
</head>
<body>
<div id="main">
<h1>The Air conditioning controller</h1>
<form method="post" action="ac.cgi">
<h2>ON/OFF</h2>
<input type="submit" name="onoff" value="toggle">
<h2>TEMPERATURE</h2>
<input type="submit" name="temp" value="16">
<input type="submit" name="temp" value="17">
<input type="submit" name="temp" value="18">
<input type="submit" name="temp" value="19">
<input type="submit" name="temp" value="20">
<input type="submit" name="temp" value="21">
<input type="submit" name="temp" value="22">
<input type="submit" name="temp" value="23">
<input type="submit" name="temp" value="24">
<input type="submit" name="temp" value="25">
<!-- max temp is 31 degrees -->
<h2>FAN</h2>
<input type="submit" name="fan" value="AUTO">
<input type="submit" name="fan" value="MAX">
<input type="submit" name="fan" value="MED">
<input type="submit" name="fan" value="MIN">
<h2>MODE</h2>
<input type="submit" name="mode" value="SUN">
<input type="submit" name="mode" value="FAN">
<input type="submit" name="mode" value="COOL">
<input type="submit" name="mode" value="SMART">
<input type="submit" name="mode" value="DROPS">
<h2>SLEEP</h2>
<input type="submit" name="sleep" value="toggle">
<!--<input type="submit" name="led" value="0">-->
</form>
</div>
</body></html>
