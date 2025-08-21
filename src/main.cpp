#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <LittleFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// === Function Declarations ===
void connectToWiFi();
void setupWebServer();
void handleRoot();
void handleSubmit(AsyncWebServerRequest *request);
void handle404();
String getFormattedDateTime();
String formatCustomDate(String customDate);
void initializePrinter();
void printReceipt();
void printServerInfo();
void setInverse(bool enable);
void printLine(String line);
void advancePaper(int lines);
void printWrappedUpsideDown(String text);

// === WiFi Configuration ===
const char *ssid = "Your WIFI name";
const char *password = "Your WIFI password";

// === Time Configuration ===
const long utcOffsetInSeconds = 0; // UTC offset in seconds (0 for UTC, 3600 for UTC+1, etc.)
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds, 60000);

// === Web Server ===
AsyncWebServer server(80);

// === Printer Setup ===
HardwareSerial printer(1);
const int TX_PIN = 21;	   // TX pin to printer RX
const int maxCharsPerLine = 32;

// === Storage for form data ===
struct Receipt
{
	String message;
	String timestamp;
	bool hasData;
};

Receipt currentReceipt = {"", "", false};

void setup()
{
	Serial.begin(115200);
	Serial.println("\n=== Thermal Printer Server Starting ===");

	if (!LittleFS.begin(true))
	{
		Serial.println("LittleFS Mount Failed");
	}

	// Initialize printer
	initializePrinter();

	// Connect to WiFi
	connectToWiFi();

	// Initialize time client
	timeClient.begin();
	Serial.println("Time client initialized");

	// Setup web server routes
	setupWebServer();

	// Start the server
	server.begin();
	Serial.println("Web server started");

	// Print server info
	printServerInfo();

	Serial.println("=== Setup Complete ===");
}

void loop()
{
	// Update time client
	timeClient.update();

	// Check if we have a new receipt to print
	if (currentReceipt.hasData)
	{
		printReceipt();
		currentReceipt.hasData = false; // Reset flag
	}

	delay(10); // Small delay to prevent excessive CPU usage
}

// === WiFi Connection ===
void connectToWiFi()
{
	Serial.print("Connecting to WiFi: ");
	Serial.println(ssid);

	WiFi.begin(ssid, password);

	int attempts = 0;
	while (WiFi.status() != WL_CONNECTED && attempts < 30)
	{
		delay(1000);
		Serial.print(".");
		attempts++;
	}

	if (WiFi.status() == WL_CONNECTED)
	{
		Serial.println();
		Serial.println("WiFi connected successfully!");
		Serial.print("IP address: ");
		Serial.println(WiFi.localIP());
	}
	else
	{
		Serial.println();
		Serial.println("Failed to connect to WiFi");
	}
}

// === Web Server Setup ===
void setupWebServer()
{
	// Serve the main page
	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
			  { request->send(LittleFS, "/html/index.html", "text/html"); });

	// Handle form submission
	server.on("/submit", HTTP_POST, handleSubmit);

	// ADD THIS LINE to also handle submission via URL
	server.on("/submit", HTTP_GET, handleSubmit);

	// Handle 404
	server.onNotFound([](AsyncWebServerRequest *request)
					  { request->send(404, "text/plain", "The requested page was not found."); });
}

void handleSubmit(AsyncWebServerRequest *request)
{
	if (request->hasArg("message"))
	{

		currentReceipt.message = request->arg("message");

		// Check if a custom date was provided
		if (request->hasArg("date"))
		{
			String customDate = request->arg("date");
			currentReceipt.timestamp = formatCustomDate(customDate);
			Serial.println("Using custom date: " + customDate);
		}
		else
		{
			currentReceipt.timestamp = getFormattedDateTime();
			Serial.println("Using current date");
		}

		currentReceipt.hasData = true;

		Serial.println("=== New Receipt Received ===");
		Serial.println("Message: " + currentReceipt.message);
		Serial.println("Time: " + currentReceipt.timestamp);
		Serial.println("============================");

		request->send(200, "text/plain", "Receipt received and will be printed!");
	}
	else
	{
		request->send(400, "text/plain", "Missing message parameter");
	}
}

// === Time Utilities ===
String getFormattedDateTime()
{
	timeClient.update();

	// Get epoch time
	unsigned long epochTime = timeClient.getEpochTime();

	// Convert to struct tm
	time_t rawTime = epochTime;
	struct tm *timeInfo = gmtime(&rawTime);

	// Day names and month names
	String dayNames[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	String monthNames[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
						   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	// Format: "Sat, 06 Jun 2025"
	String formatted = dayNames[timeInfo->tm_wday] + ", ";
	formatted += String(timeInfo->tm_mday < 10 ? "0" : "") + String(timeInfo->tm_mday) + " ";
	formatted += monthNames[timeInfo->tm_mon] + " ";
	formatted += String(timeInfo->tm_year + 1900);

	return formatted;
}

String formatCustomDate(String customDate)
{
	// Expected format: YYYY-MM-DD or DD/MM/YYYY or similar
	// This function will try to parse common date formats and return formatted string

	String dayNames[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	String monthNames[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
						   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	int day = 0, month = 0, year = 0;

	// Try to parse YYYY-MM-DD format
	if (customDate.indexOf('-') != -1)
	{
		int firstDash = customDate.indexOf('-');
		int secondDash = customDate.indexOf('-', firstDash + 1);

		if (firstDash != -1 && secondDash != -1)
		{
			year = customDate.substring(0, firstDash).toInt();
			month = customDate.substring(firstDash + 1, secondDash).toInt();
			day = customDate.substring(secondDash + 1).toInt();
		}
	}
	// Try to parse DD/MM/YYYY format
	else if (customDate.indexOf('/') != -1)
	{
		int firstSlash = customDate.indexOf('/');
		int secondSlash = customDate.indexOf('/', firstSlash + 1);

		if (firstSlash != -1 && secondSlash != -1)
		{
			day = customDate.substring(0, firstSlash).toInt();
			month = customDate.substring(firstSlash + 1, secondSlash).toInt();
			year = customDate.substring(secondSlash + 1).toInt();
		}
	}

	// Validate parsed values
	if (day < 1 || day > 31 || month < 1 || month > 12 || year < 1900 || year > 2100)
	{
		Serial.println("Invalid date format, using current date");
		return getFormattedDateTime();
	}

	// Calculate day of week (simplified algorithm - may not be 100% accurate for all dates)
	// For a more accurate calculation, you might want to use a proper date library
	int dayOfWeek = 0; // Default to Sunday if we can't calculate

	// Format: "Sat, 06 Jun 2025"
	String formatted = dayNames[dayOfWeek] + ", ";
	formatted += String(day < 10 ? "0" : "") + String(day) + " ";
	formatted += monthNames[month - 1] + " ";
	formatted += String(year);

	return formatted;
}

// === Printer Functions ===
void initializePrinter()
{
	printer.begin(115200, SERIAL_8N1, -1, TX_PIN);

	delay(500);

	// Initialise
	printer.write(0x1B);
	printer.write('@'); // ESC @
	delay(50);

	// I've found my CSN A5 doesn't need to be adjusted to work well
	// Leaving this for now.
	// // Set stronger black fill (print density/heat)
	// printer.write(0x1B); printer.write('7');
	// printer.write(15);  // Heating dots (max 15)
	// printer.write(150); // Heating time
	// printer.write(250); // Heating interval

	// Enable 180° rotation (which also reverses the line order)
	printer.write(0x1B);
	printer.write('{');
	printer.write(0x01); // ESC { 1

	Serial.println("Printer initialised");
}

void printReceipt()
{
	Serial.println("Printing receipt...");

	// Print wrapped message first (appears at bottom after rotation)
	printWrappedUpsideDown(currentReceipt.message);

	// Print header last (appears at top after rotation)
	setInverse(true);
	printLine(currentReceipt.timestamp);
	setInverse(false);

	// Advance paper
	advancePaper(2);

	Serial.println("Receipt printed successfully");
}

void printServerInfo()
{
	Serial.println("=== Server Info ===");
	Serial.print("Local IP: ");
	Serial.println(WiFi.localIP());
	Serial.print("Access the form at: http://");
	Serial.println(WiFi.localIP());
	Serial.println("==================");

	// Also print server info on the thermal printer
	Serial.println("Printing server info on thermal printer...");

	String serverInfo = "Server started at " + WiFi.localIP().toString();
	printWrappedUpsideDown(serverInfo);

	setInverse(true);
	printLine("PRINTER SERVER READY");
	setInverse(false);

	advancePaper(3);
}

// === Original Printer Helper Functions ===
void setInverse(bool enable)
{
	printer.write(0x1D);
	printer.write('B');
	printer.write(enable ? 1 : 0); // GS B n
}

void printLine(String line)
{
	printer.println(line);
}

void advancePaper(int lines)
{
	for (int i = 0; i < lines; i++)
	{
		printer.write(0x0A); // LF
	}
}

void printWrappedUpsideDown(String text)
{
	String lines[100];
	int lineCount = 0;

	while (text.length() > 0)
	{
		int breakIndex = maxCharsPerLine;
		if (text.length() <= maxCharsPerLine)
		{
			lines[lineCount++] = text;
			break;
		}

		int lastSpace = text.lastIndexOf(' ', maxCharsPerLine);
		if (lastSpace == -1)
			lastSpace = maxCharsPerLine;

		lines[lineCount++] = text.substring(0, lastSpace);
		text = text.substring(lastSpace);
		text.trim();
	}

	for (int i = lineCount - 1; i >= 0; i--)
	{
		printLine(lines[i]);
	}
}