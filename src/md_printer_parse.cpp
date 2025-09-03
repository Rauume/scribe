/*
TODO:
BLOCKS:
- [x] MD_BLOCK_H 				- Headings (H1 - H6)
- [x] MD_BLOCK_LI/ 				- Unordered List
- [ ] MD_BLOCK_OL				- Numbered List. WIP.
- [x] MD_BLOCK_HR 				- Horizontal Breaks (---, ___, ***)
- [x] MD_BLOCK_QUOTE 			- Blockquotes (>)

SPANS:
- [x] MD_SPAN_STRONG - 			- **Bold**
- [x] MD_SPAN_U - 				- __Underlined__
- [x] MD_SPAN_CODE - 			- ```Code Block```
- [~] MD_SPAN_EM - 				- *Italics*
Note: Printer has no support for Italics, without pushing a custom font.
We'll surrond the text with `*` for now.
- MD_SPAN_DEL - 				- ~~Strikethrough~~
Note: Printer doesn't officially support strikethrough text, but it CAN be bodged by
exploiting the reset command, causing text to print overtop itself.
- MD_SPAN_IMG - Image
Note: Image support would be neat, but not planned right now.

TEXT:
- [x] MD_TEXT_BR, MD_TEXT_SOFTBR - Linebreaks `\n`
- [x] MD_TEXT_CODE - 			`Inline` code block

OTHER:
- [ ] Word wrap. Stop chars overflowing to next line.
*/

#include "md_printer_parse.h"

unsigned int parser_flags = MD_FLAG_UNDERLINE | MD_FLAG_STRIKETHROUGH | MD_FLAG_HARD_SOFT_BREAKS;
HardwareSerial *pPrinter = nullptr;

const int indent_size = 16;
const int max_indent_level = 16;
int current_indent_level = 0;

const MD_PARSER parser = {
	.abi_version = 0,
	.flags = parser_flags,
	.enter_block = enterBlock,
	.leave_block = leaveBlock,
	.enter_span = enterSpan,
	.leave_span = leaveSpan,
	.text = parseText,
	// .debug_log = 
};

int enterBlock(MD_BLOCKTYPE type, void *detail, void *userdata)
{
	switch (type)
	{
		case MD_BLOCK_H:
		{
			Serial.println("Enter: Header");
			MD_BLOCK_H_DETAIL *h_detail = static_cast<MD_BLOCK_H_DETAIL *>(detail);
			
			switch (h_detail->level) {
                case 1: // H1:										3x width | 3x height
                    sendCommand(0x1D); 	sendCommand(0x21); 	sendCommand(0x20 | 0x02);
                    break;
                case 2: // H2:									   	3x width | 2x height
                    sendCommand(0x1D); 	sendCommand(0x21); 	sendCommand(0x20 | 0x01);
                    break;
                case 3: // H3: 									   	2x width | 2x height
					sendCommand(0x1D); 	sendCommand(0x21); 	sendCommand(0x10 | 0x01);                    
                    break;
                case 4: // H4: 										2x width | 1x height
                    sendCommand(0x1D); 	sendCommand(0x21); 	sendCommand(0x10 | 0x00);
                    break;
                case 5: // H5: normal text, bolded
                    sendCommand(0x1B); 	sendCommand(0x21); 	sendCommand(0x08);
                    break;
                case 6: // H6: normal text, underlined x2 pixel thick
                    sendCommand(0x1B); sendCommand(0x2D); sendCommand(0x02);
                    break;
            }
			break;
			
		}
		case MD_BLOCK_HR: //Horizontal Break
			sendText("================================", 32);
			break;
		case MD_BLOCK_QUOTE:
			sendText("> ", 2); // Print once at the start of the block quote
			setIndent(1);
			break;
		
		// LISTS
		// case MD_BLOCK_UL:
		// case MD_BLOCK_OL:
		case MD_BLOCK_LI:
			sendText("- ", 2);
            break;

		case MD_BLOCK_CODE: //Same as MD_SPAN_CODE
			MD_BLOCK_CODE_DETAIL *code_detail = static_cast<MD_BLOCK_CODE_DETAIL*>(detail);
		
			sendText("```", 3);
			sendText(code_detail->lang.text, code_detail->lang.size);
			sendNewLine();
			
			sendCommand(0x1B);
			sendCommand(0x21);
            sendCommand(0x01); // Small characters
			break;
	}
	
	return 0;
}

int leaveBlock(MD_BLOCKTYPE type, void *detail, void *userdata)
{
	switch (type) {
		case MD_BLOCK_QUOTE:
			setIndent(-1);
			break;
		case MD_BLOCK_LI:
			sendNewLine();
			break;
		case MD_BLOCK_H:
			sendCommand(0x1B);
            sendCommand(0x21);
            sendCommand(0x00); // Reset to default font
			sendNewLine();
			break;
		case MD_BLOCK_CODE:
			sendText("```", 3);
			sendCommand(0x1B);
			sendCommand(0x21);
            sendCommand(0x00);
			sendNewLine();
			break;
		// case MD_BLOCK_HTML:     std::cout << "LEAVE: HTML Block" << std::endl; break;
		// case MD_BLOCK_P:        std::cout << "LEAVE: Paragraph" << std::endl; break;
		case MD_BLOCK_P:
			sendNewLine();
			sendNewLine();
			break;
	}
	
	return 0;
}

int enterSpan(MD_SPANTYPE type, void *detail, void *userdata)
{
	switch (type)
	{
		// 'Set character printing method' csna5.pdf Page 13
		case MD_SPAN_STRONG: // Bold
			sendCommand(0x1B);
            sendCommand(0x21);
            sendCommand(0x08); // Bold
            break;      
        
        case MD_SPAN_U: // Underline
            sendCommand(0x1B);
            sendCommand(0x2D);
            sendCommand(0x01); // 01 pixel thick
            break;
            
        case MD_SPAN_DEL: // Strikethrough
            break;
			
		case MD_SPAN_CODE:
			sendText("`", 1);
			sendCommand(0x1B);
			sendCommand(0x21);
            sendCommand(0x01); // Small Characters
			break;
			
		// The printer doesn't support italics
		// So we add `*` char instead
		case MD_SPAN_EM: 
			sendText("*", 1);
			break;
	}
	return 0;
}

int leaveSpan(MD_SPANTYPE type, void *detail, void *userdata)
{
	switch (type)
    {
        case MD_SPAN_STRONG: // Bold
			sendCommand(0x1B);
            sendCommand(0x21);
            sendCommand(0x00); // Clear font
            break;

        case MD_SPAN_U: // Underline
            sendCommand(0x1B);
            sendCommand(0x2D);
            sendCommand(0x00); // Clear Underline
            break;
            
		case MD_SPAN_CODE: // Inline code block
			sendText("`", 1);		
            sendCommand(0x1B);
            sendCommand(0x21);
            sendCommand(0x00); // Clear font
            break;
		case MD_SPAN_EM:
			sendText("*", 1);
			break;
    }

	return 0;
}

int parseText(MD_TEXTTYPE type, const MD_CHAR *text, MD_SIZE size, void *userdata)
{
	switch (type)
	{		
		case MD_TEXT_BR:
		case MD_TEXT_SOFTBR:
			sendNewLine();
			break;
		
		default:
			sendText(text, size);
			break;
	}

	return 0;
}

void sendCommand(int data)
{
	pPrinter->write(data);
}

void sendText(const char *data, int size)
{
	if (size > 0)
	{
		pPrinter->write(data, size);

		// Serial.print("Printing Text: ");
		// Serial.write(data, size);
        // Serial.print(" Size: ");
		// Serial.print(size);
		// Serial.println();
	}
}
void sendNewLine()
{
	pPrinter->write("\n");
}

void dispatchMDParser(String text, HardwareSerial &printer)
{
	pPrinter = &printer;

	Serial.println("Dispatching MD parser");
	int result = md_parse(text.c_str(), text.length(), &parser, {});
	
	Serial.print("MD_RESULT: ");
	Serial.println(result);
}
				
// 'Set the left margin' Page 13
// Typically 1 for right, -1 for left
void setIndent(int direction)
{
	current_indent_level += direction;
	
	// Clamp the values
	if (current_indent_level < 0)
		current_indent_level = 0;
	else if (current_indent_level > max_indent_level)
		current_indent_level = max_indent_level;
		
	sendCommand(0x1D);
	sendCommand(0x4C);
	sendCommand(current_indent_level * indent_size);
	sendCommand(0x00);
}
