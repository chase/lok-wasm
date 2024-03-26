import { createInterface } from 'readline';
import { stdin } from 'process';

const includeList = [
  'DocumentType',
  'SelectionType',
  'CallbackType',
  'SetTextSelectionType',
  'SetGraphicSelectionType',
];

function stripLokPrefix(value) {
  const enumNameMatch = value.match(/\s*LOK_[^_]+_(\w+)/);
  if (enumNameMatch) {
    return enumNameMatch[1];
  }
}

function stripEnumNamePrefix(name) {
  return name.replace('LibreOfficeKit', '');
}

function processEnum(enumName, enumValues) {
  if (!includeList.includes(enumName)) {
    return;
  }
  console.log(`export enum ${enumName} {`);
  for (const value of enumValues) {
    if (value.comment) {
      console.log(value.comment);
    }
    if ('value' in value) {
      console.log(`  ${stripLokPrefix(value.name)} = ${value.value},`);
    } else {
      console.log(`  ${stripLokPrefix(value.name)},`);
    }
  }
  console.log('};');
}

const rl = createInterface({
  input: stdin,
});

let insideEnum = false;
let nextLineIsName = false;
let insideBlockComment = false;
let comment = null;
let enumValues = [];
let enumName = null;

rl.on('line', (line) => {
  line = line.trim();
  // Handle block comments
  if (line.includes('/*')) {
    insideBlockComment = true;
  }
  if (insideBlockComment) {
    if (line.includes('*/')) {
      insideBlockComment = false;
    }
    if (!comment) comment = '';
    comment += `  ${line}\n`;
    return; // Skip processing lines inside block comments
  }

  // Skip single-line comments
  if (line.startsWith('//')) {
    return;
  }

  // Detect the start of an enum
  if (/typedef enum/.test(line)) {
    insideEnum = true;
    return;
  }

  if (insideEnum) {
    if (line.startsWith('}')) {
      nextLineIsName = true;
      return;
    } else if (nextLineIsName) {
      // Extract the enum name and process the collected enum values
      const enumNameMatch = line.match(/\s*(\w+)[^;]*;/);
      if (enumNameMatch) {
        enumName = stripEnumNamePrefix(enumNameMatch[1]);
        processEnum(enumName, enumValues);
        insideEnum = false; // Reset for the next enum
        enumValues = []; // Clear values for the next enum
      }
      nextLineIsName = false;
    } else {
      // Collect enum values, accounting for multiple values per line
      const nameMatch = line.match(/([^\s=;,{}]+)/);
      const valueMatch = line.match(/=\s*([^;,{}]+)/);
      if (valueMatch && nameMatch) {
        enumValues.push({
          name: nameMatch[1],
          value: valueMatch[1],
          comment,
        });
      } else if (nameMatch) {
        enumValues.push({ name: nameMatch[1], comment });
      }
    }
    comment = null;
  }
});
