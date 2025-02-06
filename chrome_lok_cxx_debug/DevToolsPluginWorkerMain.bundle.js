import createSymbolsBackend from './SymbolsBackend.js';

// Copyright 2023 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
class MemorySlice {
    begin;
    buffer;
    constructor(buffer, begin) {
        this.begin = begin;
        this.buffer = buffer;
    }
    merge(other) {
        if (other.begin < this.begin) {
            return other.merge(this);
        }
        if (other.begin > this.end) {
            throw new Error('Slices are not contiguous');
        }
        if (other.end <= this.end) {
            return this;
        }
        const newBuffer = new Uint8Array(other.end - this.begin);
        newBuffer.set(new Uint8Array(this.buffer), 0);
        newBuffer.set(new Uint8Array(other.buffer, this.end - other.begin), this.length);
        return new MemorySlice(newBuffer.buffer, this.begin);
    }
    contains(offset) {
        return this.begin <= offset && offset < this.end;
    }
    get length() {
        return this.buffer.byteLength;
    }
    get end() {
        return this.length + this.begin;
    }
    view(begin, length) {
        return new DataView(this.buffer, begin - this.begin, length);
    }
}
class PageStore {
    slices = [];
    // Returns the highest index |i| such that |slices[i].start <= offset|, or -1 if there is no such |i|.
    findSliceIndex(offset) {
        let begin = 0;
        let end = this.slices.length;
        while (begin < end) {
            const idx = Math.floor((end + begin) / 2);
            const pivot = this.slices[idx];
            if (offset < pivot.begin) {
                end = idx;
            }
            else {
                begin = idx + 1;
            }
        }
        return begin - 1;
    }
    findSlice(offset) {
        return this.getSlice(this.findSliceIndex(offset), offset);
    }
    getSlice(index, offset) {
        if (index < 0) {
            return null;
        }
        const candidate = this.slices[index];
        return candidate?.contains(offset) ? candidate : null;
    }
    addSlice(buffer, begin) {
        let slice = new MemorySlice(Array.isArray(buffer) ? new Uint8Array(buffer).buffer : buffer, begin);
        let leftPosition = this.findSliceIndex(slice.begin - 1);
        const leftOverlap = this.getSlice(leftPosition, slice.begin - 1);
        if (leftOverlap) {
            slice = slice.merge(leftOverlap);
        }
        else {
            leftPosition++;
        }
        const rightPosition = this.findSliceIndex(slice.end);
        const rightOverlap = this.getSlice(rightPosition, slice.end);
        if (rightOverlap) {
            slice = slice.merge(rightOverlap);
        }
        this.slices.splice(leftPosition, // Insert to the right if no overlap
        rightPosition - leftPosition + 1, // Delete one additional slice if overlapping on the left
        slice);
        return slice;
    }
}
class WasmMemoryView {
    wasm;
    pages = new PageStore();
    static PAGE_SIZE = 4096;
    constructor(wasm) {
        this.wasm = wasm;
    }
    page(byteOffset, byteLength) {
        const mask = WasmMemoryView.PAGE_SIZE - 1;
        const offset = byteOffset & mask;
        const page = byteOffset - offset;
        const rangeEnd = byteOffset + byteLength;
        const count = 1 + Math.ceil((rangeEnd - (rangeEnd & mask) - page) / WasmMemoryView.PAGE_SIZE);
        return { page, offset, count };
    }
    getPages(page, count) {
        if (page & (WasmMemoryView.PAGE_SIZE - 1)) {
            throw new Error('Not a valid page');
        }
        let slice = this.pages.findSlice(page);
        const size = WasmMemoryView.PAGE_SIZE * count;
        if (!slice || slice.length < count * WasmMemoryView.PAGE_SIZE) {
            const data = this.wasm.readMemory(page, size);
            if (data.byteOffset !== 0 || data.byteLength !== data.buffer.byteLength) {
                throw new Error('Did not expect a partial memory view');
            }
            slice = this.pages.addSlice(data.buffer, page);
        }
        return slice.view(page, size);
    }
    getFloat32(byteOffset, littleEndian) {
        const { offset, page, count } = this.page(byteOffset, 4);
        const view = this.getPages(page, count);
        return view.getFloat32(offset, littleEndian);
    }
    getFloat64(byteOffset, littleEndian) {
        const { offset, page, count } = this.page(byteOffset, 8);
        const view = this.getPages(page, count);
        return view.getFloat64(offset, littleEndian);
    }
    getInt8(byteOffset) {
        const { offset, page, count } = this.page(byteOffset, 1);
        const view = this.getPages(page, count);
        return view.getInt8(offset);
    }
    getInt16(byteOffset, littleEndian) {
        const { offset, page, count } = this.page(byteOffset, 2);
        const view = this.getPages(page, count);
        return view.getInt16(offset, littleEndian);
    }
    getInt32(byteOffset, littleEndian) {
        const { offset, page, count } = this.page(byteOffset, 4);
        const view = this.getPages(page, count);
        return view.getInt32(offset, littleEndian);
    }
    getUint8(byteOffset) {
        const { offset, page, count } = this.page(byteOffset, 1);
        const view = this.getPages(page, count);
        return view.getUint8(offset);
    }
    getUint16(byteOffset, littleEndian) {
        const { offset, page, count } = this.page(byteOffset, 2);
        const view = this.getPages(page, count);
        return view.getUint16(offset, littleEndian);
    }
    getUint32(byteOffset, littleEndian) {
        const { offset, page, count } = this.page(byteOffset, 4);
        const view = this.getPages(page, count);
        return view.getUint32(offset, littleEndian);
    }
    getBigInt64(byteOffset, littleEndian) {
        const { offset, page, count } = this.page(byteOffset, 8);
        const view = this.getPages(page, count);
        return view.getBigInt64(offset, littleEndian);
    }
    getBigUint64(byteOffset, littleEndian) {
        const { offset, page, count } = this.page(byteOffset, 8);
        const view = this.getPages(page, count);
        return view.getBigUint64(offset, littleEndian);
    }
    asDataView(byteOffset, byteLength) {
        const { offset, page, count } = this.page(byteOffset, byteLength);
        const view = this.getPages(page, count);
        return new DataView(view.buffer, view.byteOffset + offset, byteLength);
    }
}
const globalTypeMap = new Map();
class CXXValue {
    location;
    type;
    data;
    memoryOrDataView;
    wasm;
    typeMap;
    memoryView;
    membersMap;
    objectStore;
    objectId;
    displayValue;
    memoryAddress;
    constructor(objectStore, wasm, memoryView, location, type, typeMap, data, displayValue, memoryAddress) {
        if (!location && !data) {
            throw new Error('Cannot represent nullptr');
        }
        this.data = data;
        this.location = location;
        this.type = type;
        this.typeMap = typeMap;
        this.wasm = wasm;
        this.memoryOrDataView = data ? new DataView(new Uint8Array(data).buffer) : memoryView;
        if (data && data.length !== type.size) {
            throw new Error('Invalid data size');
        }
        this.memoryView = memoryView;
        this.objectStore = objectStore;
        this.objectId = objectStore.store(this);
        this.displayValue = displayValue;
        this.memoryAddress = memoryAddress;
    }
    static create(objectStore, wasm, memoryView, typeInfo) {
        const typeMap = new Map();
        for (const info of typeInfo.typeInfos) {
            typeMap.set(info.typeId, info);
            for (const name of info.typeNames) {
                if (name.endsWith("::XInteractionHandler2")) {
                    console.log(name, info);
                }
                globalTypeMap.set(name, info);
            }
        }
        const { location, root, data, displayValue, memoryAddress } = typeInfo;
        return new CXXValue(objectStore, wasm, memoryView, location ?? 0, root, typeMap, data, displayValue, memoryAddress);
    }
    get members() {
        if (!this.membersMap) {
            this.membersMap = new Map();
            for (const member of this.type.members) {
                const memberType = this.typeMap.get(member.typeId);
                if (memberType && member.name) {
                    const memberLocation = member.name === '*' ? this.memoryOrDataView.getUint32(this.location, true) :
                        this.location + member.offset;
                    this.membersMap.set(member.name, { location: memberLocation, type: memberType });
                }
            }
        }
        return this.membersMap;
    }
    getArrayElement(index) {
        const data = this.members.has('*') ? undefined : this.data;
        const element = this.members.get('*') || this.members.get('0');
        if (!element) {
            throw new Error(`Incomplete type information for array or pointer type '${this.typeNames}'`);
        }
        return new CXXValue(this.objectStore, this.wasm, this.memoryView, element.location + index * element.type.size, element.type, this.typeMap, data);
    }
    async getProperties() {
        const properties = [];
        if (this.type.arraySize > 0) {
            for (let index = 0; index < this.type.arraySize; ++index) {
                properties.push({ name: `${index}`, property: await this.getArrayElement(index) });
            }
        }
        else {
            const members = await this.members;
            const data = members.has('*') ? undefined : this.data;
            for (const [name, { location, type }] of members) {
                const property = new CXXValue(this.objectStore, this.wasm, this.memoryView, location, type, this.typeMap, data);
                properties.push({ name, property });
            }
        }
        return properties;
    }
    async asRemoteObject() {
        if (this.type.hasValue && this.type.arraySize === 0) {
            const formatter = CustomFormatters.get(this.type);
            if (!formatter) {
                const type = 'undefined';
                const description = '<not displayable>';
                return { type, description, hasChildren: false };
            }
            if (this.location === undefined || (!this.data && this.location === 0xffffffff)) {
                const type = 'undefined';
                const description = '<optimized out>';
                return { type, description, hasChildren: false };
            }
            const value = new CXXValue(this.objectStore, this.wasm, this.memoryView, this.location, this.type, this.typeMap, this.data);
            try {
                const formattedValue = await formatter.format(this.wasm, value);
                return lazyObjectFromAny(formattedValue, this.objectStore, this.type, this.displayValue, this.memoryAddress)
                    .asRemoteObject();
            }
            catch {
                // Fallthrough
            }
        }
        const type = (this.type.arraySize > 0 ? 'array' : 'object');
        const { objectId } = this;
        return {
            type,
            description: this.type.typeNames[0],
            hasChildren: this.type.members.length > 0,
            linearMemoryAddress: this.memoryAddress,
            linearMemorySize: this.type.size,
            objectId,
        };
    }
    get typeNames() {
        return this.type.typeNames;
    }
    get size() {
        return this.type.size;
    }
    asInt8() {
        return this.memoryOrDataView.getInt8(this.location);
    }
    asInt16() {
        return this.memoryOrDataView.getInt16(this.location, true);
    }
    asInt32() {
        return this.memoryOrDataView.getInt32(this.location, true);
    }
    asInt64() {
        return this.memoryOrDataView.getBigInt64(this.location, true);
    }
    asUint8() {
        return this.memoryOrDataView.getUint8(this.location);
    }
    asUint16() {
        return this.memoryOrDataView.getUint16(this.location, true);
    }
    asUint32() {
        return this.memoryOrDataView.getUint32(this.location, true);
    }
    asUint64() {
        return this.memoryOrDataView.getBigUint64(this.location, true);
    }
    asFloat32() {
        return this.memoryOrDataView.getFloat32(this.location, true);
    }
    asFloat64() {
        return this.memoryOrDataView.getFloat64(this.location, true);
    }
    asDataView(offset, size) {
        offset = this.location + (offset ?? 0);
        size = size ?? this.size;
        if (this.memoryOrDataView instanceof DataView) {
            size = Math.min(size - offset, this.memoryOrDataView.byteLength - offset - this.location);
            if (size < 0) {
                throw new RangeError('Size exceeds the buffer range');
            }
            return new DataView(this.memoryOrDataView.buffer, this.memoryOrDataView.byteOffset + this.location + offset, size);
        }
        return this.memoryView.asDataView(offset, size);
    }
    $(selector) {
        const data = this.members.has('*') ? undefined : this.data;
        if (typeof selector === 'number') {
            return this.getArrayElement(selector);
        }
        const dot = selector.indexOf('.');
        const memberName = dot >= 0 ? selector.substring(0, dot) : selector;
        selector = selector.substring(memberName.length + 1);
        const member = this.members.get(memberName);
        if (!member) {
            throw new Error(`Type ${this.typeNames[0] || '<anonymous>'} has no member '${memberName}'. Available members are: ${Array.from(this.members.keys())}`);
        }
        const memberValue = new CXXValue(this.objectStore, this.wasm, this.memoryView, member.location, member.type, this.typeMap, data);
        if (selector.length === 0) {
            return memberValue;
        }
        return memberValue.$(selector);
    }
    getMembers() {
        return Array.from(this.members.keys());
    }
    castTo(typeName) {
        const targetType = globalTypeMap.get(typeName);
        if (!targetType) {
            throw new Error(`Type '${typeName}' not found in type information`);
        }
        return new CXXValue(this.objectStore, this.wasm, this.memoryView, this.location, targetType, this.typeMap, this.data, this.displayValue, this.memoryAddress);
    }
    castChildAtIndexTo(index, typeName) {
        const targetType = globalTypeMap.get(typeName);
        if (!targetType) {
            throw new Error(`Type '${typeName}' not found in type information`);
        }
        const ptr = this.location + index * targetType.size;
        return new CXXValue(this.objectStore, this.wasm, this.memoryView, ptr, targetType, this.typeMap);
    }
}
function primitiveObject(value, description, linearMemoryAddress, type) {
    if (['number', 'string', 'boolean', 'bigint', 'undefined'].includes(typeof value)) {
        if (typeof value === 'bigint' || typeof value === 'number') {
            const enumerator = type?.enumerators?.find(e => e.value === BigInt(value));
            if (enumerator) {
                description = enumerator.name;
            }
        }
        return new PrimitiveLazyObject(typeof value, value, description, linearMemoryAddress, type?.size);
    }
    return null;
}
function lazyObjectFromAny(value, objectStore, type, description, linearMemoryAddress) {
    const primitive = primitiveObject(value, description, linearMemoryAddress, type);
    if (primitive) {
        return primitive;
    }
    if (value instanceof CXXValue) {
        return value;
    }
    if (typeof value === 'object') {
        if (value === null) {
            return new PrimitiveLazyObject('null', value, description, linearMemoryAddress);
        }
        return new LocalLazyObject(value, objectStore, type, linearMemoryAddress);
    }
    if (typeof value === 'function') {
        return value();
    }
    throw new Error('Value type is not formattable');
}
class LazyObjectStore {
    nextObjectId = 0;
    objects = new Map();
    store(lazyObject) {
        const objectId = `${this.nextObjectId++}`;
        this.objects.set(objectId, lazyObject);
        return objectId;
    }
    get(objectId) {
        return this.objects.get(objectId);
    }
    release(objectId) {
        this.objects.delete(objectId);
    }
    clear() {
        this.objects.clear();
    }
}
class PrimitiveLazyObject {
    type;
    value;
    description;
    linearMemoryAddress;
    linearMemorySize;
    constructor(type, value, description, linearMemoryAddress, linearMemorySize) {
        this.type = type;
        this.value = value;
        this.description = description ?? `${value}`;
        this.linearMemoryAddress = linearMemoryAddress;
        this.linearMemorySize = linearMemorySize;
    }
    async getProperties() {
        return [];
    }
    async asRemoteObject() {
        const { type, value, description, linearMemoryAddress, linearMemorySize } = this;
        return { type, hasChildren: false, value, description, linearMemoryAddress, linearMemorySize };
    }
}
class LocalLazyObject {
    value;
    objectId;
    objectStore;
    type;
    linearMemoryAddress;
    constructor(value, objectStore, type, linearMemoryAddress) {
        this.value = value;
        this.objectStore = objectStore;
        this.objectId = objectStore.store(this);
        this.type = type;
        this.linearMemoryAddress = linearMemoryAddress;
    }
    async getProperties() {
        return Object.entries(this.value).map(([name, value]) => {
            const property = lazyObjectFromAny(value, this.objectStore);
            return { name, property };
        });
    }
    async asRemoteObject() {
        const type = (Array.isArray(this.value) ? 'array' : 'object');
        const { objectId, type: valueType, linearMemoryAddress } = this;
        return {
            type,
            objectId,
            description: valueType?.typeNames[0],
            hasChildren: Object.keys(this.value).length > 0,
            linearMemorySize: valueType?.size,
            linearMemoryAddress,
        };
    }
}
class HostWasmInterface {
    hostInterface;
    stopId;
    cache = [];
    view;
    constructor(hostInterface, stopId) {
        this.hostInterface = hostInterface;
        this.stopId = stopId;
        this.view = new WasmMemoryView(this);
    }
    readMemory(offset, length) {
        return new Uint8Array(this.hostInterface.getWasmLinearMemory(offset, length, this.stopId));
    }
    getOp(op) {
        return this.hostInterface.getWasmOp(op, this.stopId);
    }
    getLocal(local) {
        return this.hostInterface.getWasmLocal(local, this.stopId);
    }
    getGlobal(global) {
        return this.hostInterface.getWasmGlobal(global, this.stopId);
    }
}
class DebuggerProxy {
    wasm;
    target;
    constructor(wasm, target) {
        this.wasm = wasm;
        this.target = target;
    }
    readMemory(src, dst, length) {
        const data = this.wasm.view.asDataView(src, length);
        this.target.HEAP8.set(new Uint8Array(data.buffer, data.byteOffset, length), dst);
        return data.byteLength;
    }
    getLocal(index) {
        return this.wasm.getLocal(index);
    }
    getGlobal(index) {
        return this.wasm.getGlobal(index);
    }
    getOperand(index) {
        return this.wasm.getOp(index);
    }
}
class CustomFormatters {
    static formatters = new Map();
    static genericFormatters = [];
    static addFormatter(formatter) {
        if (Array.isArray(formatter.types)) {
            for (const type of formatter.types) {
                CustomFormatters.formatters.set(type, formatter);
            }
        }
        else {
            CustomFormatters.genericFormatters.push(formatter);
        }
    }
    static get(type) {
        for (const name of type.typeNames) {
            const formatter = CustomFormatters.formatters.get(name);
            if (formatter) {
                return formatter;
            }
        }
        for (const t of type.typeNames) {
            const CONST_PREFIX = 'const ';
            if (t.startsWith(CONST_PREFIX)) {
                const formatter = CustomFormatters.formatters.get(t.substr(CONST_PREFIX.length));
                if (formatter) {
                    return formatter;
                }
            }
        }
        for (const formatter of CustomFormatters.genericFormatters) {
            if (formatter.types instanceof Function) {
                if (formatter.types(type)) {
                    return formatter;
                }
            }
        }
        return null;
    }
}

// Copyright 2023 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
/*
 * Numbers
 */
CustomFormatters.addFormatter({
    types: ["bool"],
    format: (wasm, value) => value.asUint8() > 0,
});
CustomFormatters.addFormatter({
    types: ["uint16_t"],
    format: (wasm, value) => value.asUint16(),
});
CustomFormatters.addFormatter({
    types: ["uint32_t"],
    format: (wasm, value) => value.asUint32(),
});
CustomFormatters.addFormatter({
    types: ["uint64_t"],
    format: (wasm, value) => value.asUint64(),
});
CustomFormatters.addFormatter({
    types: ["int16_t"],
    format: (wasm, value) => value.asInt16(),
});
CustomFormatters.addFormatter({
    types: ["int32_t"],
    format: (wasm, value) => value.asInt32(),
});
CustomFormatters.addFormatter({
    types: ["int64_t"],
    format: (wasm, value) => value.asInt64(),
});
CustomFormatters.addFormatter({
    types: ["float"],
    format: (wasm, value) => value.asFloat32(),
});
CustomFormatters.addFormatter({
    types: ["double"],
    format: (wasm, value) => value.asFloat64(),
});
function formatVoid() {
    return () => new PrimitiveLazyObject("undefined", undefined, "<void>");
}
CustomFormatters.addFormatter({ types: ["void"], format: formatVoid });
CustomFormatters.addFormatter({
    types: ["uint8_t", "int8_t"],
    format: formatChar,
});
function formatChar(wasm, value) {
    const char = value.typeNames.includes("int8_t")
        ? Math.abs(value.asInt8())
        : value.asUint8();
    switch (char) {
        case 0x0:
            return "'\\0'";
        case 0x7:
            return "'\\a'";
        case 0x8:
            return "'\\b'";
        case 0x9:
            return "'\\t'";
        case 0xa:
            return "'\\n'";
        case 0xb:
            return "'\\v'";
        case 0xc:
            return "'\\f'";
        case 0xd:
            return "'\\r'";
    }
    if (char < 0x20 || char > 0x7e) {
        return `'\\x${char.toString(16).padStart(2, "0")}'`;
    }
    return `'${String.fromCharCode(value.asInt8())}'`;
}
CustomFormatters.addFormatter({
    types: ["wchar_t", "char32_t", "char16_t"],
    format: (wasm, value) => {
        const codepoint = value.size === 2 ? value.asUint16() : value.asUint32();
        try {
            return String.fromCodePoint(codepoint);
        }
        catch {
            return `U+${codepoint.toString(16).padStart(value.size * 2, "0")}`;
        }
    },
});
/*
 * STL
 */
function formatLibCXXString(wasm, value, charType, decode) {
    const shortString = value.$("__r_.__value_.<union>.__s");
    const size = shortString.getMembers().includes("<union>")
        ? shortString.$("<union>.__size_").asUint8()
        : shortString.$("__size_").asUint8();
    const isLong = 0 < (size & 0x80);
    const charSize = charType.BYTES_PER_ELEMENT;
    if (isLong) {
        const longString = value.$("__r_.__value_.<union>.__l");
        const data = longString.$("__data_").asUint32();
        const stringSize = longString.$("__size_").asUint32();
        const copyLen = Math.min(stringSize * charSize, 268435440 /* Constants.MAX_STRING_LEN */);
        const bytes = wasm.readMemory(data, copyLen);
        const text = new charType(bytes.buffer, bytes.byteOffset, stringSize);
        return { size: stringSize, string: decode(text) };
    }
    const bytes = shortString.$("__data_").asDataView(0, size * charSize);
    const text = new charType(bytes.buffer, bytes.byteOffset, size);
    return { size, string: decode(text) };
}
function formatLibCXX8String(wasm, value) {
    return formatLibCXXString(wasm, value, Uint8Array, (str) => new TextDecoder().decode(str));
}
function formatLibCXX16String(wasm, value) {
    return formatLibCXXString(wasm, value, Uint16Array, (str) => new TextDecoder("utf-16le").decode(str));
}
function formatLibCXX32String(wasm, value) {
    // emscripten's wchar is 4 byte
    return formatLibCXXString(wasm, value, Uint32Array, (str) => Array.from(str)
        .map((v) => String.fromCodePoint(v))
        .join(""));
}
CustomFormatters.addFormatter({
    types: [
        "std::__2::string",
        "std::__2::basic_string<char, std::__2::char_traits<char>, std::__2::allocator<char> >",
        "std::__2::u8string",
        "std::__2::basic_string<char8_t, std::__2::char_traits<char8_t>, std::__2::allocator<char8_t> >",
    ],
    format: formatLibCXX8String,
});
CustomFormatters.addFormatter({
    types: [
        "std::__2::u16string",
        "std::__2::basic_string<char16_t, std::__2::char_traits<char16_t>, std::__2::allocator<char16_t> >",
    ],
    format: formatLibCXX16String,
});
CustomFormatters.addFormatter({
    types: [
        "std::__2::wstring",
        "std::__2::basic_string<wchar_t, std::__2::char_traits<wchar_t>, std::__2::allocator<wchar_t> >",
        "std::__2::u32string",
        "std::__2::basic_string<char32_t, std::__2::char_traits<char32_t>, std::__2::allocator<char32_t> >",
    ],
    format: formatLibCXX32String,
});
function formatStringBuffer(wasm, buffer, length, encoding = "utf-8", bytesPerChar = 1) {
    if (!buffer || buffer.asUint32() === 0 || length === 0) {
        return "";
    }
    const isLong = 0 < (length & 0x80);
    const charType = bytesPerChar === 2
        ? Uint16Array
        : bytesPerChar === 4
            ? Uint32Array
            : Uint8Array;
    if (isLong) {
        const data = buffer.asUint32();
        const copyLen = Math.min(length * bytesPerChar, 268435440 /* Constants.MAX_STRING_LEN */);
        const bytes = wasm.readMemory(data, copyLen);
        const text = new charType(bytes.buffer, bytes.byteOffset, length);
        return { size: length, string: new TextDecoder(encoding).decode(text) };
    }
    const bytes = buffer.asDataView(0, length * bytesPerChar);
    const text = new charType(bytes.buffer, bytes.byteOffset, length);
    return new TextDecoder(encoding).decode(text);
}
function formatRTLStringBase(wasm, value, encoding = "utf-8", bytesPerChar = 1) {
    const buffer = value.$("buffer");
    const length = value.$("length").asUint32();
    return formatStringBuffer(wasm, buffer, length, encoding, bytesPerChar);
}
// Base RTL object string formatter
function formatRTLOStringBase(wasm, value, encoding = "utf-8", bytesPerChar = 1) {
    return value.$("pData");
}
// Specific RTL string formatters using the base implementations
function formatRTLString(wasm, value) {
    return formatRTLStringBase(wasm, value);
}
function formatRTLUString(wasm, value) {
    return formatRTLStringBase(wasm, value, "utf-16le", 2);
}
function formatRTLOString(wasm, value) {
    return formatRTLOStringBase(wasm, value);
}
function formatRTLOUString(wasm, value) {
    return formatRTLOStringBase(wasm, value, "utf-16", 2);
}
// Register the RTL string formatters with consolidated type lists
CustomFormatters.addFormatter({
    types: ["_rtl_String", "rtl::OStringBuffer"],
    format: formatRTLString,
});
CustomFormatters.addFormatter({
    types: ["_rtl_uString", "rtl::OUStringBuffer"],
    format: formatRTLUString,
});
CustomFormatters.addFormatter({
    types: reMatch(/^rtl::OString$/),
    format: formatRTLOString,
});
CustomFormatters.addFormatter({
    types: reMatch(/^rtl::OUString$/),
    format: formatRTLOUString,
});
CustomFormatters.addFormatter({
    types: ["rtl_uString *", "rtl_String *"],
    format: (wasm, value) => value.$("*"),
});
const PRIMITIVE_TO_RESULT_VIEW = {
    [1 /* TypeClass.CHAR */]: (value) => String.fromCharCode(value.asInt8()),
    [2 /* TypeClass.BOOLEAN */]: (value) => value.asUint8() !== 0,
    [3 /* TypeClass.BYTE */]: (value) => String.fromCharCode(value.asUint8()),
    [4 /* TypeClass.SHORT */]: (value) => value.asInt16(),
    [5 /* TypeClass.UNSIGNED_SHORT */]: (value) => value.asUint16(),
    [6 /* TypeClass.LONG */]: (value) => value.asInt32(),
    [7 /* TypeClass.UNSIGNED_LONG */]: (value) => value.asUint32(),
    [8 /* TypeClass.HYPER */]: (value) => value.asInt64(),
    [9 /* TypeClass.UNSIGNED_HYPER */]: (value) => value.asUint64(),
    [10 /* TypeClass.FLOAT */]: (value) => value.asFloat32(),
    [11 /* TypeClass.DOUBLE */]: (value) => value.asFloat64(),
    [12 /* TypeClass.STRING */]: (value, wasm) => {
        const strValue = value.castTo("rtl::OUString");
        return formatRTLOUString(wasm, strValue);
    },
};
// Mapping primitive UNO types to C++ types
const PRIMITIVE_TO_CPP = {
    [13 /* TypeClass.TYPE */]: "com::sun::star::uno::Type",
    [14 /* TypeClass.ANY */]: "com::sun::star::uno::Any",
};
// UNO to C++ type set
const UNO_TO_CPP = new Set([
    15 /* TypeClass.ENUM */,
    17 /* TypeClass.STRUCT */,
    19 /* TypeClass.EXCEPTION */,
    22 /* TypeClass.INTERFACE */,
]);
// Constants
const CSSU_TYPE = "com::sun::star::uno::Type";
const TYPE_DESC = "_typelib_TypeDescription";
const TYPE_DESCS = new Set([
    TYPE_DESC,
    "_typelib_CompoundTypeDescription",
    "_typelib_StructTypeDescription",
    "_typelib_IndirectTypeDescription",
    "_typelib_EnumTypeDescription",
    "_typelib_InterfaceMemberTypeDescription",
    "_typelib_InterfaceMethodTypeDescription",
    "_typelib_InterfaceAttributeTypeDescription",
    "_typelib_InterfaceTypeDescription",
]);
const TYPE_DESC_REF = "_typelib_TypeDescriptionReference";
class TypeEntry {
    type_class;
    uno_type;
    cpp_type;
    element_type;
    constructor(type_class, uno_type, cpp_type, element_type) {
        this.type_class = type_class;
        this.uno_type = uno_type;
        this.cpp_type = cpp_type;
        this.element_type = element_type;
    }
}
// Cache for type resolution
const unresolved_type_cache = new Set();
const resolved_type_cache = new Map();
function unoToCpp(uno) {
    return uno.replace(/\./g, "::");
}
function resolveUnoType(value, wasm) {
    const address = value.location;
    if (unresolved_type_cache.has(address)) {
        return undefined;
    }
    if (resolved_type_cache.has(address)) {
        return resolved_type_cache.get(address);
    }
    let val = value;
    let typeNames = value.typeNames;
    if (typeNames.includes(CSSU_TYPE)) {
        const pValue = value.$("_pType");
        unresolved_type_cache.add(address);
        if (!pValue) {
            return undefined;
        }
        val = pValue.$("*");
    }
    while (typeNames.includes(TYPE_DESC_REF)) {
        const pValue = val.$("pType");
        if (!pValue) {
            return undefined;
        }
        val = pValue.$("*");
        typeNames = val.typeNames;
    }
    if (!val.typeNames.some((name) => TYPE_DESCS.has(name))) {
        unresolved_type_cache.add(address);
        return undefined;
    }
    let fullVal = val;
    if (!val.typeNames.includes(TYPE_DESC)) {
        while (val.$("aBase")) {
            val = val.$("aBase");
        }
    }
    const typeClass = val.$("eTypeClass").asUint32();
    const nameRef = val.$("pTypeName").$("*");
    const nameString = formatRTLUString(wasm, nameRef);
    if (typeClass in PRIMITIVE_TO_CPP) {
        const entry = new TypeEntry(typeClass, nameString, PRIMITIVE_TO_CPP[typeClass]);
        resolved_type_cache.set(address, entry);
        return entry;
    }
    else if (UNO_TO_CPP.has(typeClass)) {
        const entry = new TypeEntry(typeClass, nameString, unoToCpp(nameString));
        resolved_type_cache.set(address, entry);
        return entry;
    }
    else if (typeClass === 26 /* TypeClass.INTERFACE_ATTRIBUTE */ ||
        typeClass === 25 /* TypeClass.INTERFACE_METHOD */) {
        const [interface_, , member] = nameString.split("::");
        const entry = new TypeEntry(typeClass, nameString, `${unoToCpp(interface_)}::*${member}`);
        resolved_type_cache.set(address, entry);
        return entry;
    }
    else if (typeClass === 20 /* TypeClass.SEQUENCE */) {
        const pElem = fullVal.$("pType").$("*");
        if (!pElem) {
            unresolved_type_cache.add(address);
            return undefined;
        }
        const elem = resolveUnoType(pElem, wasm);
        if (!elem) {
            unresolved_type_cache.add(address);
            return undefined;
        }
        const entry = new TypeEntry(typeClass, nameString, `com::sun::star::uno::Sequence<${elem.cpp_type}>`, elem);
        resolved_type_cache.set(address, entry);
        return entry;
    }
    unresolved_type_cache.add(address);
    return undefined;
}
function formatUnoAny(wasm, value) {
    try {
        const typeDesc = value.$("pType");
        if (!typeDesc) {
            return undefined;
        }
        const typeClass = typeDesc.$("*").$("eTypeClass").asUint32();
        if (typeClass in PRIMITIVE_TO_RESULT_VIEW) {
            return PRIMITIVE_TO_RESULT_VIEW[typeClass](value.$("pData").$("*"), wasm);
        }
        else if (typeClass === 0 /* TypeClass.VOID */) {
            return undefined;
        }
        const type = resolveUnoType(typeDesc.$("*"), wasm);
        if (!type) {
            return undefined;
        }
        const ptr = value.$("pData");
        if (!ptr) {
            return undefined;
        }
        return ptr.castChildAtIndexTo(0, type.cpp_type);
    }
    catch (e) {
        console.error(e);
        return undefined;
    }
}
function unwrapTemplateTypeName(typeName) {
    return typeName.replace(/^.*<([^>]+)>$/, '$1');
}
function formatUnoReference(wasm, value) {
    const iface = value.$("_pInterface");
    if (!iface) {
        return undefined;
    }
    return iface.$("*").castTo(unwrapTemplateTypeName(value.typeNames[0]));
}
function formatUnoSequence(wasm, value) {
    const ptr = value.$("_pSequence");
    if (!ptr) {
        return [];
    }
    const impl = ptr.$("*");
    const size = impl.$("nElements").asUint32();
    const elements = impl.$("elements");
    const result = [];
    for (let i = 0; i < size; i++) {
        result.push(elements.$(i));
    }
    return result;
}
function formatUnoSequencePropertyValue(wasm, value) {
    const ptr = value.$("_pSequence");
    if (!ptr) {
        return {};
    }
    const impl = ptr.$("*");
    const size = impl.$("nElements").asUint32();
    const elements = impl.$("elements");
    const result = {};
    for (let i = 0; i < size; i++) {
        const el = elements.castChildAtIndexTo(i, "com::sun::star::beans::PropertyValue");
        const nameRef = el.$("Name").$("pData").$("*");
        const nameString = formatRTLStringBase(wasm, nameRef, "utf-16le", 2);
        result[nameString] = el.$("Value");
    }
    return result;
}
function formatUnoType(wasm, value) {
    return resolveUnoType(value, wasm) ? value : undefined;
}
// Register formatters
CustomFormatters.addFormatter({
    types: reMatch(/^_uno_Any$/, /^com::sun::star::uno::Any$/),
    format: formatUnoAny,
});
CustomFormatters.addFormatter({
    types: reMatch(/^com::sun::star::uno::Reference<.+>$/),
    format: formatUnoReference,
});
// special case for map-like
CustomFormatters.addFormatter({
    types: reMatch(/^com::sun::star::uno::Sequence<com::sun::star::beans::PropertyValue>$/),
    format: formatUnoSequencePropertyValue,
});
CustomFormatters.addFormatter({
    types: reMatch(/^com::sun::star::uno::Sequence<.+>$/),
    format: formatUnoSequence,
});
CustomFormatters.addFormatter({
    types: reMatch(/^com::sun::star::uno::Type$/),
    format: formatUnoType,
});
function formatRawString(wasm, value, charType, decode) {
    const address = value.asUint32();
    if (address < 1024 /* Constants.SAFE_HEAP_START */) {
        return formatPointerOrReference(wasm, value);
    }
    const charSize = charType.BYTES_PER_ELEMENT;
    const slices = [];
    const deref = value.$("*");
    for (let bufferSize = 0; bufferSize < 268435440 /* Constants.MAX_STRING_LEN */; bufferSize += 4096 /* Constants.PAGE_SIZE */) {
        // Copy PAGE_SIZE bytes
        const buffer = deref.asDataView(bufferSize, 4096 /* Constants.PAGE_SIZE */);
        // Convert to charType
        const substr = new charType(buffer.buffer, buffer.byteOffset, buffer.byteLength / charSize);
        const strlen = substr.indexOf(0);
        if (strlen >= 0) {
            // buffer size is in bytes, strlen in characters
            const str = new charType(bufferSize / charSize + strlen);
            for (let i = 0; i < slices.length; ++i) {
                str.set(new charType(slices[i].buffer, slices[i].byteOffset, slices[i].byteLength / charSize), (i * 4096 /* Constants.PAGE_SIZE */) / charSize);
            }
            str.set(substr.subarray(0, strlen), bufferSize / charSize);
            return decode(str);
        }
        slices.push(buffer);
    }
    return formatPointerOrReference(wasm, value);
}
function formatCString(wasm, value) {
    return formatRawString(wasm, value, Uint8Array, (str) => new TextDecoder().decode(str));
}
function formatU16CString(wasm, value) {
    return formatRawString(wasm, value, Uint16Array, (str) => new TextDecoder("utf-16le").decode(str));
}
function formatCWString(wasm, value) {
    // emscripten's wchar is 4 byte
    return formatRawString(wasm, value, Uint32Array, (str) => Array.from(str)
        .map((v) => String.fromCodePoint(v))
        .join(""));
}
// Register with higher precedence than the generic pointer handler.
CustomFormatters.addFormatter({
    types: ["char *", "char8_t *"],
    format: formatCString,
});
CustomFormatters.addFormatter({
    types: ["char16_t *"],
    format: formatU16CString,
});
CustomFormatters.addFormatter({
    types: ["wchar_t *", "char32_t *"],
    format: formatCWString,
});
function formatVector(wasm, value) {
    const begin = value.$("__begin_");
    const end = value.$("__end_");
    const size = (end.asUint32() - begin.asUint32()) / begin.$("*").size;
    const elements = [];
    for (let i = 0; i < size; ++i) {
        elements.push(begin.$(i));
    }
    return elements;
}
function reMatch(...exprs) {
    return (type) => {
        for (const expr of exprs) {
            for (const name of type.typeNames) {
                if (expr.exec(name)) {
                    return true;
                }
            }
        }
        for (const expr of exprs) {
            for (const name of type.typeNames) {
                if (name.startsWith("const ")) {
                    if (expr.exec(name.substring(6))) {
                        return true;
                    }
                }
            }
        }
        return false;
    };
}
CustomFormatters.addFormatter({
    types: reMatch(/^std::vector<.+>$/),
    format: formatVector,
});
function formatPointerOrReference(wasm, value) {
    const address = value.asUint32();
    if (address === 0) {
        return { "0x0": null };
    }
    return { [`0x${address.toString(16)}`]: value.$("*") };
}
CustomFormatters.addFormatter({
    types: (type) => type.isPointer,
    format: formatPointerOrReference,
});
function formatDynamicArray(wasm, value) {
    return { [`0x${value.location.toString(16)}`]: value.$(0) };
}
CustomFormatters.addFormatter({
    types: reMatch(/^.+\[\]$/),
    format: formatDynamicArray,
});
function formatUInt128(wasm, value) {
    const view = value.asDataView();
    return ((view.getBigUint64(8, true) << BigInt(64)) + view.getBigUint64(0, true));
}
CustomFormatters.addFormatter({
    types: ["unsigned __int128"],
    format: formatUInt128,
});
function formatInt128(wasm, value) {
    const view = value.asDataView();
    return (view.getBigInt64(8, true) << BigInt(64)) | view.getBigUint64(0, true);
}
CustomFormatters.addFormatter({ types: ["__int128"], format: formatInt128 });
function formatExternRef(wasm, value) {
    const obj = {
        async getProperties() {
            return [];
        },
        async asRemoteObject() {
            const encodedValue = value.asUint64();
            const ValueClasses = [
                "global",
                "local",
                "operand",
            ];
            const valueClass = ValueClasses[Number(encodedValue >> 32n)];
            return {
                type: "reftype",
                valueClass,
                index: Number(BigInt.asUintN(32, encodedValue)),
            };
        },
    };
    return () => obj;
}
CustomFormatters.addFormatter({
    types: ["__externref_t", "externref_t"],
    format: formatExternRef,
});

// Copyright 2023 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
function globToRegExp(glob) {
    let re = '^';
    for (let i = 0; i < glob.length; ++i) {
        const c = glob.charCodeAt(i);
        if (c === 0x2a) {
            if (i + 2 < glob.length && glob.charCodeAt(i + 1) === 0x2a && glob.charCodeAt(i + 2) === 0x2f) {
                // Compile '**/' to match everything including slashes.
                re += '.*';
                i += 2;
            }
            else {
                // Compile '*' to match everything except slashes.
                re += '[^/]*';
            }
        }
        else {
            // Just escape everything else, so we don't need to
            // worry about special characters like ., +, $, etc.
            re += `\\u${c.toString(16).padStart(4, '0')}`;
        }
    }
    re += '$';
    return new RegExp(re, 'i');
}
/**
 * Performs a glob-style pattern matching.
 *
 * The following special characters are supported for the `pattern`:
 *
 * - '*' matches every sequence of characters, except for slash ('/').
 * - '**' plus '/' matches every sequence of characters, including slash ('/').
 *
 * If the `pattern` doesn't contain a slash ('/'), only the last path
 * component of the `subject` (its basename) will be matched against
 * the `pattern`. Otherwise if at least one slash is found in `pattern`
 * the full `subject` is matched against the `pattern`.
 *
 * @param pattern the wildcard pattern
 * @param subject the subject URL to test against
 * @return whether the `subject` matches the given `pattern`.
 */
function globMatch(pattern, subject) {
    const regexp = globToRegExp(pattern);
    if (!pattern.includes('/')) {
        subject = subject.slice(subject.lastIndexOf('/') + 1);
    }
    return regexp.test(subject);
}

// Copyright 2023 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
/**
 * Resolve a source path (as stored in DWARF debugging information) to an absolute URL.
 *
 * Note that we treat "." specially as a pattern, since LLDB normalizes paths before
 * returning them from the DWARF parser. Our logic replicates the logic found in the
 * LLDB frontend in `PathMappingList::RemapPath()` inside `Target/PathMappingList.cpp`
 * (http://cs/github/llvm/llvm-project/lldb/source/Target/PathMappingList.cpp?l=157-185).
 *
 * @param pathSubstitutions possible substitutions to apply to the {@param sourcePath}, applies the first match.
 * @param sourcePath the source path as found in the debugging information.
 * @param baseURL the URL of the WebAssembly module, which is used to resolve relative source paths.
 * @return an absolute `file:`-URI or a URL relative to the {@param baseURL}.
 */
function resolveSourcePathToURL(pathSubstitutions, sourcePath, baseURL) {
    // Normalize '\' to '/' in sourcePath first.
    let resolvedSourcePath = sourcePath.replace(/\\/g, '/');
    // Apply source path substitutions first.
    for (const { from, to } of pathSubstitutions) {
        if (resolvedSourcePath.startsWith(from)) {
            resolvedSourcePath = to + resolvedSourcePath.slice(from.length);
            break;
        }
        // Relative paths won't have a leading "./" in them unless "." is the only
        // thing in the relative path so we need to work around "." carefully.
        if (from === '.') {
            // We need to figure whether sourcePath can be considered a relative path,
            // ruling out absolute POSIX and Windows paths, as well as file:, http: and
            // https: URLs.
            if (!resolvedSourcePath.startsWith('/') && !/^([A-Z]|file|https?):/i.test(resolvedSourcePath)) {
                resolvedSourcePath = `${to}/${resolvedSourcePath}`;
                break;
            }
        }
    }
    if (resolvedSourcePath.startsWith('/')) {
        if (resolvedSourcePath.startsWith('//')) {
            return new URL(`file:${resolvedSourcePath}`);
        }
        return new URL(`file://${resolvedSourcePath}`);
    }
    if (/^[A-Z]:/i.test(resolvedSourcePath)) {
        return new URL(`file:/${resolvedSourcePath}`);
    }
    return new URL(resolvedSourcePath, baseURL.href);
}
/**
 * Locate the configuration for a given `something.wasm` module file name.
 *
 * @param moduleConfigurations list of module configurations to scan.
 * @param moduleName the URL of the module to lookup.
 * @return the matching module configuration or the default fallback.
 */
function findModuleConfiguration(moduleConfigurations, moduleURL) {
    let defaultModuleConfiguration = { pathSubstitutions: [] };
    for (const moduleConfiguration of moduleConfigurations) {
        // The idea here is that module configurations will have at most
        // one default configuration, so picking the last here is fine.
        if (moduleConfiguration.name === undefined) {
            defaultModuleConfiguration = moduleConfiguration;
            continue;
        }
        // Perform wildcard pattern matching on the full URL.
        if (globMatch(moduleConfiguration.name, moduleURL.href)) {
            return moduleConfiguration;
        }
    }
    return defaultModuleConfiguration;
}
const DEFAULT_MODULE_CONFIGURATIONS = [{ pathSubstitutions: [] }];

// Copyright 2023 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
function mapVector(vector, callback) {
    const elements = [];
    for (let i = 0; i < vector.size(); ++i) {
        const element = vector.get(i);
        elements.push(callback(element));
    }
    return elements;
}
function mapEnumerator(apiEnumerator) {
    return { typeId: apiEnumerator.typeId, value: apiEnumerator.value, name: apiEnumerator.name };
}
function mapFieldInfo(apiFieldInfo) {
    return { typeId: apiFieldInfo.typeId, offset: apiFieldInfo.offset, name: apiFieldInfo.name };
}
class ModuleInfo {
    symbolsUrl;
    symbolsFileName;
    symbolsDwpFileName;
    backend;
    fileNameToUrl;
    urlToFileName;
    dwarfSymbolsPlugin;
    constructor(symbolsUrl, symbolsFileName, symbolsDwpFileName, backend) {
        this.symbolsUrl = symbolsUrl;
        this.symbolsFileName = symbolsFileName;
        this.symbolsDwpFileName = symbolsDwpFileName;
        this.backend = backend;
        this.fileNameToUrl = new Map();
        this.urlToFileName = new Map();
        this.dwarfSymbolsPlugin = new backend.DWARFSymbolsPlugin();
    }
    stringifyScope(scope) {
        switch (scope) {
            case this.backend.VariableScope.GLOBAL:
                return 'GLOBAL';
            case this.backend.VariableScope.LOCAL:
                return 'LOCAL';
            case this.backend.VariableScope.PARAMETER:
                return 'PARAMETER';
        }
        throw new Error(`InternalError: Invalid scope ${scope}`);
    }
    stringifyErrorCode(errorCode) {
        switch (errorCode) {
            case this.backend.ErrorCode.PROTOCOL_ERROR:
                return 'ProtocolError:';
            case this.backend.ErrorCode.MODULE_NOT_FOUND_ERROR:
                return 'ModuleNotFoundError:';
            case this.backend.ErrorCode.INTERNAL_ERROR:
                return 'InternalError';
            case this.backend.ErrorCode.EVAL_ERROR:
                return 'EvalError';
        }
        throw new Error(`InternalError: Invalid error code ${errorCode}`);
    }
}
function createEmbindPool() {
    class EmbindObjectPool {
        objectPool = [];
        flush() {
            for (const object of this.objectPool.reverse()) {
                object.delete();
            }
            this.objectPool = [];
        }
        manage(object) {
            if (typeof object !== 'undefined') {
                this.objectPool.push(object);
            }
            return object;
        }
        unmanage(object) {
            const index = this.objectPool.indexOf(object);
            if (index > -1) {
                this.objectPool.splice(index, 1);
                object.delete();
                return true;
            }
            return false;
        }
    }
    const pool = new EmbindObjectPool();
    const manage = pool.manage.bind(pool);
    const unmanage = pool.unmanage.bind(pool);
    const flush = pool.flush.bind(pool);
    return { manage, unmanage, flush };
}
// Cache the underlying WebAssembly module after the first instantiation
// so that subsequent calls to `createSymbolsBackend()` are faster, which
// greatly speeds up the test suite.
let symbolsBackendModulePromise;
function instantiateWasm(imports, callback, resourceLoader) {
    if (!symbolsBackendModulePromise) {
        symbolsBackendModulePromise = resourceLoader.createSymbolsBackendModulePromise();
    }
    symbolsBackendModulePromise.then(module => WebAssembly.instantiate(module, imports))
        .then(callback)
        .catch(console.error);
    return [];
}
class DWARFLanguageExtensionPlugin {
    moduleConfigurations;
    resourceLoader;
    hostInterface;
    moduleInfos = new Map();
    lazyObjects = new LazyObjectStore();
    constructor(moduleConfigurations, resourceLoader, hostInterface) {
        this.moduleConfigurations = moduleConfigurations;
        this.resourceLoader = resourceLoader;
        this.hostInterface = hostInterface;
        this.moduleConfigurations = moduleConfigurations;
    }
    async newModuleInfo(rawModuleId, symbolsHint, rawModule) {
        const { flush, manage } = createEmbindPool();
        try {
            const rawModuleURL = new URL(rawModule.url);
            const { pathSubstitutions } = findModuleConfiguration(this.moduleConfigurations, rawModuleURL);
            const symbolsURL = symbolsHint ? resolveSourcePathToURL([], symbolsHint, rawModuleURL) : rawModuleURL;
            const instantiateWasmWrapper = (imports, callback) => {
                // Emscripten type definitions are incorrect, we're getting passed a WebAssembly.Imports object here.
                return instantiateWasm(imports, callback, this.resourceLoader);
            };
            const backend = await createSymbolsBackend({ instantiateWasm: instantiateWasmWrapper });
            const { symbolsFileName, symbolsDwpFileName } = await this.resourceLoader.loadSymbols(rawModuleId, rawModule, symbolsURL, backend.FS, this.hostInterface);
            const moduleInfo = new ModuleInfo(symbolsURL.href, symbolsFileName, symbolsDwpFileName, backend);
            const addRawModuleResponse = manage(moduleInfo.dwarfSymbolsPlugin.AddRawModule(rawModuleId, symbolsFileName));
            mapVector(manage(addRawModuleResponse.sources), fileName => {
                const fileURL = resolveSourcePathToURL(pathSubstitutions, fileName, symbolsURL);
                moduleInfo.fileNameToUrl.set(fileName, fileURL.href);
                moduleInfo.urlToFileName.set(fileURL.href, fileName);
            });
            // Set up lazy dwo files if we are running on a worker
            if (typeof global === 'undefined' && typeof importScripts === 'function' &&
                typeof XMLHttpRequest !== 'undefined') {
                mapVector(manage(addRawModuleResponse.dwos), dwoFile => {
                    const absolutePath = dwoFile.startsWith('/') ? dwoFile : '/' + dwoFile;
                    const pathSplit = absolutePath.split('/');
                    const fileName = pathSplit.pop();
                    const parentDirectory = pathSplit.join('/');
                    // Sometimes these stick around.
                    try {
                        backend.FS.unlink(absolutePath);
                    }
                    catch {
                    }
                    // Ensure directory exists
                    if (parentDirectory.length > 1) {
                        // TypeScript doesn't know about createPath
                        // @ts-ignore
                        backend.FS.createPath('/', parentDirectory.substring(1), true, true);
                    }
                    const dwoURL = new URL(dwoFile, symbolsURL).href;
                    const node = backend.FS.createLazyFile(parentDirectory, fileName, dwoURL, true, false);
                    const cacheLength = node.contents.cacheLength;
                    const wrapper = () => {
                        try {
                            cacheLength.apply(node.contents);
                            void this.hostInterface.reportResourceLoad(dwoURL, { success: true, size: node.contents.length });
                        }
                        catch (e) {
                            void this.hostInterface.reportResourceLoad(dwoURL, { success: false, errorMessage: e.message });
                            // Rethrow any error fetching the content as errno 44 (EEXIST)
                            // TypeScript doesn't know about the ErrnoError constructor
                            // @ts-ignore
                            throw new backend.FS.ErrnoError(44);
                        }
                    };
                    node.contents.cacheLength = wrapper;
                });
            }
            return moduleInfo;
        }
        finally {
            flush();
        }
    }
    async addRawModule(rawModuleId, symbolsUrl, rawModule) {
        // This complex logic makes sure that addRawModule / removeRawModule calls are
        // handled sequentially for the same rawModuleId, and thus this looks symmetrical
        // to the removeRawModule() method below. The idea is that we chain our operation
        // on any previous operation for the same rawModuleId, and thereby end up with a
        // single sequence of events.
        const originalPromise = Promise.resolve(this.moduleInfos.get(rawModuleId));
        const moduleInfoPromise = originalPromise.then(moduleInfo => {
            if (moduleInfo) {
                throw new Error(`InternalError: Duplicate module with ID '${rawModuleId}'`);
            }
            return this.newModuleInfo(rawModuleId, symbolsUrl, rawModule);
        });
        // This looks a bit odd, but it's important that the operation is chained via
        // the `_moduleInfos` map *and* at the same time resolves to it's original
        // value in case of an error (i.e. if someone tried to add the same rawModuleId
        // twice, this will retain the original value in that case instead of having all
        // users get the internal error).
        this.moduleInfos.set(rawModuleId, moduleInfoPromise.catch(() => originalPromise));
        const moduleInfo = await moduleInfoPromise;
        return [...moduleInfo.urlToFileName.keys()];
    }
    async getModuleInfo(rawModuleId) {
        const moduleInfo = await this.moduleInfos.get(rawModuleId);
        if (!moduleInfo) {
            throw new Error(`InternalError: Unknown module with raw module ID ${rawModuleId}`);
        }
        return moduleInfo;
    }
    async removeRawModule(rawModuleId) {
        const originalPromise = Promise.resolve(this.moduleInfos.get(rawModuleId));
        const moduleInfoPromise = originalPromise.then(moduleInfo => {
            if (!moduleInfo) {
                throw new Error(`InternalError: No module with ID '${rawModuleId}'`);
            }
            return undefined;
        });
        this.moduleInfos.set(rawModuleId, moduleInfoPromise.catch(() => originalPromise));
        await moduleInfoPromise;
    }
    async sourceLocationToRawLocation(sourceLocation) {
        const { flush, manage } = createEmbindPool();
        const moduleInfo = await this.getModuleInfo(sourceLocation.rawModuleId);
        const sourceFile = moduleInfo.urlToFileName.get(sourceLocation.sourceFileURL);
        if (!sourceFile) {
            throw new Error(`InternalError: Unknown URL ${sourceLocation.sourceFileURL}`);
        }
        try {
            const rawLocations = manage(moduleInfo.dwarfSymbolsPlugin.SourceLocationToRawLocation(sourceLocation.rawModuleId, sourceFile, sourceLocation.lineNumber, sourceLocation.columnNumber));
            const error = manage(rawLocations.error);
            if (error) {
                throw new Error(`${moduleInfo.stringifyErrorCode(error.code)}: ${error.message}`);
            }
            const locations = mapVector(manage(rawLocations.rawLocationRanges), rawLocation => {
                const { rawModuleId, startOffset, endOffset } = manage(rawLocation);
                return { rawModuleId, startOffset, endOffset };
            });
            return locations;
        }
        finally {
            flush();
        }
    }
    async rawLocationToSourceLocation(rawLocation) {
        const { flush, manage } = createEmbindPool();
        const moduleInfo = await this.getModuleInfo(rawLocation.rawModuleId);
        try {
            const sourceLocations = moduleInfo.dwarfSymbolsPlugin.RawLocationToSourceLocation(rawLocation.rawModuleId, rawLocation.codeOffset, rawLocation.inlineFrameIndex || 0);
            const error = manage(sourceLocations.error);
            if (error) {
                throw new Error(`${moduleInfo.stringifyErrorCode(error.code)}: ${error.message}`);
            }
            const locations = mapVector(manage(sourceLocations.sourceLocation), sourceLocation => {
                const sourceFileURL = moduleInfo.fileNameToUrl.get(sourceLocation.sourceFile);
                if (!sourceFileURL) {
                    throw new Error(`InternalError: Unknown source file ${sourceLocation.sourceFile}`);
                }
                const { rawModuleId, lineNumber, columnNumber } = manage(sourceLocation);
                return {
                    rawModuleId,
                    sourceFileURL,
                    lineNumber,
                    columnNumber,
                };
            });
            return locations;
        }
        finally {
            flush();
        }
    }
    async getScopeInfo(type) {
        switch (type) {
            case 'GLOBAL':
                return {
                    type,
                    typeName: 'Global',
                };
            case 'LOCAL':
                return {
                    type,
                    typeName: 'Local',
                };
            case 'PARAMETER':
                return {
                    type,
                    typeName: 'Parameter',
                };
        }
        throw new Error(`InternalError: Invalid scope type '${type}`);
    }
    async listVariablesInScope(rawLocation) {
        const { flush, manage } = createEmbindPool();
        const moduleInfo = await this.getModuleInfo(rawLocation.rawModuleId);
        try {
            const variables = manage(moduleInfo.dwarfSymbolsPlugin.ListVariablesInScope(rawLocation.rawModuleId, rawLocation.codeOffset, rawLocation.inlineFrameIndex || 0));
            const error = manage(variables.error);
            if (error) {
                throw new Error(`${moduleInfo.stringifyErrorCode(error.code)}: ${error.message}`);
            }
            const apiVariables = mapVector(manage(variables.variable), variable => {
                const { scope, name, type } = manage(variable);
                return { scope: moduleInfo.stringifyScope(scope), name, type, nestedName: name.split('::') };
            });
            return apiVariables;
        }
        finally {
            flush();
        }
    }
    async getFunctionInfo(rawLocation) {
        const { flush, manage } = createEmbindPool();
        const moduleInfo = await this.getModuleInfo(rawLocation.rawModuleId);
        try {
            const functionInfo = manage(moduleInfo.dwarfSymbolsPlugin.GetFunctionInfo(rawLocation.rawModuleId, rawLocation.codeOffset));
            const error = manage(functionInfo.error);
            if (error) {
                throw new Error(`${moduleInfo.stringifyErrorCode(error.code)}: ${error.message}`);
            }
            const apiFunctionInfos = mapVector(manage(functionInfo.functionNames), functionName => {
                return { name: functionName };
            });
            let apiMissingSymbolFiles = mapVector(manage(functionInfo.missingSymbolFiles), x => x);
            if (apiMissingSymbolFiles.length && this.resourceLoader.possiblyMissingSymbols) {
                apiMissingSymbolFiles = apiMissingSymbolFiles.concat(this.resourceLoader.possiblyMissingSymbols);
            }
            return {
                frames: apiFunctionInfos,
                missingSymbolFiles: apiMissingSymbolFiles.map(x => new URL(x, moduleInfo.symbolsUrl).href)
            };
        }
        finally {
            flush();
        }
    }
    async getInlinedFunctionRanges(rawLocation) {
        const { flush, manage } = createEmbindPool();
        const moduleInfo = await this.getModuleInfo(rawLocation.rawModuleId);
        try {
            const rawLocations = manage(moduleInfo.dwarfSymbolsPlugin.GetInlinedFunctionRanges(rawLocation.rawModuleId, rawLocation.codeOffset));
            const error = manage(rawLocations.error);
            if (error) {
                throw new Error(`${moduleInfo.stringifyErrorCode(error.code)}: ${error.message}`);
            }
            const locations = mapVector(manage(rawLocations.rawLocationRanges), rawLocation => {
                const { rawModuleId, startOffset, endOffset } = manage(rawLocation);
                return { rawModuleId, startOffset, endOffset };
            });
            return locations;
        }
        finally {
            flush();
        }
    }
    async getInlinedCalleesRanges(rawLocation) {
        const { flush, manage } = createEmbindPool();
        const moduleInfo = await this.getModuleInfo(rawLocation.rawModuleId);
        try {
            const rawLocations = manage(moduleInfo.dwarfSymbolsPlugin.GetInlinedCalleesRanges(rawLocation.rawModuleId, rawLocation.codeOffset));
            const error = manage(rawLocations.error);
            if (error) {
                throw new Error(`${moduleInfo.stringifyErrorCode(error.code)}: ${error.message}`);
            }
            const locations = mapVector(manage(rawLocations.rawLocationRanges), rawLocation => {
                const { rawModuleId, startOffset, endOffset } = manage(rawLocation);
                return { rawModuleId, startOffset, endOffset };
            });
            return locations;
        }
        finally {
            flush();
        }
    }
    async getValueInfo(expression, context, stopId) {
        const { manage, unmanage, flush } = createEmbindPool();
        const moduleInfo = await this.getModuleInfo(context.rawModuleId);
        try {
            const apiRawLocation = manage(new moduleInfo.backend.RawLocation());
            apiRawLocation.rawModuleId = context.rawModuleId;
            apiRawLocation.codeOffset = context.codeOffset;
            apiRawLocation.inlineFrameIndex = context.inlineFrameIndex || 0;
            const wasm = new HostWasmInterface(this.hostInterface, stopId);
            const proxy = new DebuggerProxy(wasm, moduleInfo.backend);
            const typeInfoResult = manage(moduleInfo.dwarfSymbolsPlugin.EvaluateExpression(apiRawLocation, expression, proxy));
            const error = manage(typeInfoResult.error);
            if (error) {
                if (error.code === moduleInfo.backend.ErrorCode.MODULE_NOT_FOUND_ERROR) {
                    // Let's not throw when the module gets unloaded - that is quite common path that
                    // we hit when the source-scope pane still keeps asynchronously updating while we
                    // unload the wasm module.
                    return null;
                }
                // TODO(crbug.com/1271147) Instead of throwing, we whould create an AST error node with the message
                // so that it is properly surfaced to the user. This should then make the special handling of
                // MODULE_NOT_FOUND_ERROR unnecessary.
                throw new Error(`${moduleInfo.stringifyErrorCode(error.code)}: ${error.message}`);
            }
            const typeInfos = mapVector(manage(typeInfoResult.typeInfos), typeInfo => fromApiTypeInfo(manage(typeInfo)));
            const root = fromApiTypeInfo(manage(typeInfoResult.root));
            const { location, displayValue, memoryAddress } = typeInfoResult;
            const data = typeInfoResult.data ? mapVector(manage(typeInfoResult.data), n => n) : undefined;
            return { typeInfos, root, location, data, displayValue, memoryAddress };
            function fromApiTypeInfo(apiTypeInfo) {
                const apiMembers = manage(apiTypeInfo.members);
                const members = mapVector(apiMembers, fieldInfo => mapFieldInfo(manage(fieldInfo)));
                const apiEnumerators = manage(apiTypeInfo.enumerators);
                const enumerators = mapVector(apiEnumerators, enumerator => mapEnumerator(manage(enumerator)));
                unmanage(apiEnumerators);
                const typeNames = mapVector(manage(apiTypeInfo.typeNames), e => e);
                unmanage(apiMembers);
                const { typeId, size, arraySize, alignment, canExpand, isPointer, hasValue } = apiTypeInfo;
                const formatter = CustomFormatters.get({
                    typeNames,
                    typeId,
                    size,
                    alignment,
                    isPointer,
                    canExpand,
                    arraySize: arraySize ?? 0,
                    hasValue,
                    members,
                    enumerators,
                });
                return {
                    typeNames,
                    isPointer,
                    typeId,
                    size,
                    alignment,
                    canExpand: canExpand && !formatter,
                    arraySize: arraySize ?? 0,
                    hasValue: hasValue || Boolean(formatter),
                    members,
                    enumerators,
                };
            }
        }
        finally {
            flush();
        }
    }
    async getMappedLines(rawModuleId, sourceFileURL) {
        const { flush, manage } = createEmbindPool();
        const moduleInfo = await this.getModuleInfo(rawModuleId);
        const sourceFile = moduleInfo.urlToFileName.get(sourceFileURL);
        if (!sourceFile) {
            throw new Error(`InternalError: Unknown URL ${sourceFileURL}`);
        }
        try {
            const mappedLines = manage(moduleInfo.dwarfSymbolsPlugin.GetMappedLines(rawModuleId, sourceFile));
            const error = manage(mappedLines.error);
            if (error) {
                throw new Error(`${moduleInfo.stringifyErrorCode(error.code)}: ${error.message}`);
            }
            const lines = mapVector(manage(mappedLines.MappedLines), l => l);
            return lines;
        }
        finally {
            flush();
        }
    }
    async evaluate(expression, context, stopId) {
        const valueInfo = await this.getValueInfo(expression, context, stopId);
        if (!valueInfo) {
            return null;
        }
        const wasm = new HostWasmInterface(this.hostInterface, stopId);
        const cxxObject = await CXXValue.create(this.lazyObjects, wasm, wasm.view, valueInfo);
        if (!cxxObject) {
            return {
                type: 'undefined',
                hasChildren: false,
                description: '<optimized out>',
            };
        }
        return cxxObject.asRemoteObject();
    }
    async getProperties(objectId) {
        const remoteObject = this.lazyObjects.get(objectId);
        if (!remoteObject) {
            return [];
        }
        const properties = await remoteObject.getProperties();
        const descriptors = [];
        for (const { name, property } of properties) {
            descriptors.push({ name, value: await property.asRemoteObject() });
        }
        return descriptors;
    }
    async releaseObject(objectId) {
        this.lazyObjects.release(objectId);
    }
}
async function createPlugin(hostInterface, resourceLoader, moduleConfigurations = DEFAULT_MODULE_CONFIGURATIONS, logPluginApiCalls = false) {
    const plugin = new DWARFLanguageExtensionPlugin(moduleConfigurations, resourceLoader, hostInterface);
    if (logPluginApiCalls) {
        const pluginLoggingProxy = {
            get: function (target, key) {
                if (typeof target[key] === 'function') {
                    return function () {
                        const args = [...arguments];
                        const jsonArgs = args.map(x => {
                            try {
                                return JSON.stringify(x);
                            }
                            catch {
                                return x.toString();
                            }
                        })
                            .join(', ');
                        // eslint-disable-next-line no-console
                        console.info(`${key}(${jsonArgs})`);
                        return target[key].apply(target, arguments);
                    };
                }
                return Reflect.get(target, key);
            },
        };
        return new Proxy(plugin, pluginLoggingProxy);
    }
    return plugin;
}

// Copyright 2023 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
/* eslint-enable @typescript-eslint/naming-convention, @typescript-eslint/prefer-enum-initializers */
function serializeWasmValue(value, buffer) {
    if (value instanceof ArrayBuffer) {
        const data = new Uint8Array(value);
        new Uint8Array(buffer).set(data);
        return data.byteLength || -1;
    }
    const view = new DataView(buffer);
    switch (value.type) {
        case 'i32': {
            view.setInt32(0, value.value, true);
            return 1 /* SerializedWasmType.i32 */;
        }
        case 'i64': {
            view.setBigInt64(0, value.value, true);
            return 2 /* SerializedWasmType.i64 */;
        }
        case 'f32': {
            view.setFloat32(0, value.value, true);
            return 3 /* SerializedWasmType.f32 */;
        }
        case 'f64': {
            view.setFloat64(0, value.value, true);
            return 4 /* SerializedWasmType.f64 */;
        }
        case 'v128': {
            const [, a, b, c, d] = value.value.split(' ');
            view.setInt32(0, Number(a), true);
            view.setInt32(4, Number(b), true);
            view.setInt32(8, Number(c), true);
            view.setInt32(12, Number(d), true);
            return 5 /* SerializedWasmType.v128 */;
        }
        case 'reftype': {
            view.setUint8(0, ['local', 'global', 'operand'].indexOf(value.valueClass));
            view.setUint32(1, value.index, true);
            return 6 /* SerializedWasmType.reftype */;
        }
        default:
            throw new Error('cannot serialize non-numerical wasm type');
    }
}
function deserializeWasmMemory(buffer) {
    const result = new Uint8Array(buffer.byteLength);
    result.set(new Uint8Array(buffer));
    return result.buffer;
}
function deserializeWasmValue(buffer, type) {
    const view = new DataView(buffer);
    switch (type) {
        case 1 /* SerializedWasmType.i32 */:
            return { type: 'i32', value: view.getInt32(0, true) };
        case 2 /* SerializedWasmType.i64 */:
            return { type: 'i64', value: view.getBigInt64(0, true) };
        case 3 /* SerializedWasmType.f32 */:
            return { type: 'f32', value: view.getFloat32(0, true) };
        case 4 /* SerializedWasmType.f64 */:
            return { type: 'f64', value: view.getFloat64(0, true) };
        case 5 /* SerializedWasmType.v128 */: {
            const a = view.getUint32(0, true);
            const b = view.getUint32(4, true);
            const c = view.getUint32(8, true);
            const d = view.getUint32(12, true);
            return {
                type: 'v128',
                value: `i32x4 0x${a.toString(16).padStart(8, '0')} 0x${b.toString(16).padStart(8, '0')} 0x${c.toString(16).padStart(8, '0')} 0x${d.toString(16).padStart(8, '0')}`
            };
        }
        case 6 /* SerializedWasmType.reftype */: {
            const ValueClasses = ['local', 'global', 'operand'];
            const valueClass = ValueClasses[view.getUint8(0)];
            const index = view.getUint32(1, true);
            return { type: 'reftype', valueClass, index };
        }
    }
    // @ts-expect-error
    throw new Error('Invalid primitive wasm type');
}
const kMaxWasmValueSize = 4 + 4 + 4 * 10;

// Copyright 2023 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
class SynchronousIOMessage {
    buffer;
    constructor(bufferSize) {
        this.buffer = new SharedArrayBuffer(bufferSize);
    }
    static serialize(value, buffer) {
        return serializeWasmValue(value, buffer);
    }
}
/* eslint-disable-next-line @typescript-eslint/no-explicit-any */
class WorkerRPC {
    nextRequestId = 0;
    channel;
    localHandler;
    requests = new Map();
    semaphore;
    constructor(channel, localHandler) {
        this.channel = channel;
        this.channel.onmessage = this.onmessage.bind(this);
        this.localHandler = localHandler;
        this.semaphore = new Int32Array(new SharedArrayBuffer(4));
    }
    sendMessage(method, ...params) {
        const requestId = this.nextRequestId++;
        const promise = new Promise((resolve, reject) => {
            this.requests.set(requestId, { resolve, reject });
        });
        this.channel.postMessage({ requestId, request: { method, params } });
        return promise;
    }
    sendMessageSync(message, method, ...params) {
        const requestId = this.nextRequestId++;
        Atomics.store(this.semaphore, 0, 0);
        this.channel.postMessage({
            requestId,
            sync_request: {
                request: { method, params },
                io_buffer: { semaphore: this.semaphore.buffer, data: message.buffer },
            },
        });
        while (Atomics.wait(this.semaphore, 0, 0) !== 'not-equal') {
        }
        const [response] = this.semaphore;
        return message.deserialize(response);
    }
    async onmessage(event) {
        if ('request' in event.data) {
            const { requestId, request } = event.data;
            try {
                const response = await this.localHandler[request.method](...request.params);
                this.channel.postMessage({ requestId, response });
            }
            catch (error) {
                this.channel.postMessage({ requestId, error: `${error}` });
            }
        }
        else if ('sync_request' in event.data) {
            /* eslint-disable-next-line @typescript-eslint/naming-convention */
            const { sync_request: { request, io_buffer } } = event.data;
            let signal = -1;
            try {
                const response = await this.localHandler[request.method](...request.params);
                signal = SynchronousIOMessage.serialize(response, io_buffer.data);
            }
            catch (error) {
                throw error;
            }
            finally {
                const semaphore = new Int32Array(io_buffer.semaphore);
                Atomics.store(semaphore, 0, signal);
                Atomics.notify(semaphore, 0);
            }
        }
        else {
            const { requestId } = event.data;
            const callbacks = this.requests.get(requestId);
            if (callbacks) {
                const { resolve, reject } = callbacks;
                if ('error' in event.data) {
                    reject(new Error(event.data.error));
                }
                else {
                    resolve(event.data.response);
                }
            }
        }
    }
}

// Copyright 2023 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
class SynchronousLinearMemoryMessage extends SynchronousIOMessage {
    deserialize(length) {
        if (length !== this.buffer.byteLength) {
            throw new Error('Expected length to match the internal buffer size');
        }
        return deserializeWasmMemory(this.buffer);
    }
}
class SynchronousWasmValueMessage extends SynchronousIOMessage {
    deserialize(type) {
        return deserializeWasmValue(this.buffer, type);
    }
}
class RPCInterface {
    rpc;
    #plugin;
    resourceLoader;
    get plugin() {
        if (!this.#plugin) {
            throw new Error('Worker is not yet initialized');
        }
        return this.#plugin;
    }
    constructor(port, resourceLoader) {
        this.rpc = new WorkerRPC(port, this);
        this.resourceLoader = resourceLoader;
    }
    getWasmLinearMemory(offset, length, stopId) {
        return this.rpc.sendMessageSync(new SynchronousLinearMemoryMessage(length), 'getWasmLinearMemory', offset, length, stopId);
    }
    getWasmLocal(local, stopId) {
        return this.rpc.sendMessageSync(new SynchronousWasmValueMessage(kMaxWasmValueSize), 'getWasmLocal', local, stopId);
    }
    getWasmGlobal(global, stopId) {
        return this.rpc.sendMessageSync(new SynchronousWasmValueMessage(kMaxWasmValueSize), 'getWasmGlobal', global, stopId);
    }
    getWasmOp(op, stopId) {
        return this.rpc.sendMessageSync(new SynchronousWasmValueMessage(kMaxWasmValueSize), 'getWasmOp', op, stopId);
    }
    reportResourceLoad(resourceUrl, status) {
        return this.rpc.sendMessage('reportResourceLoad', resourceUrl, status);
    }
    evaluate(expression, context, stopId) {
        if (this.plugin.evaluate) {
            return this.plugin.evaluate(expression, context, stopId);
        }
        return Promise.resolve(null);
    }
    getProperties(objectId) {
        if (this.plugin.getProperties) {
            return this.plugin.getProperties(objectId);
        }
        return Promise.resolve([]);
    }
    releaseObject(objectId) {
        if (this.plugin.releaseObject) {
            return this.plugin.releaseObject(objectId);
        }
        return Promise.resolve();
    }
    addRawModule(rawModuleId, symbolsURL, rawModule) {
        return this.plugin.addRawModule(rawModuleId, symbolsURL, rawModule);
    }
    sourceLocationToRawLocation(sourceLocation) {
        return this.plugin.sourceLocationToRawLocation(sourceLocation);
    }
    rawLocationToSourceLocation(rawLocation) {
        return this.plugin.rawLocationToSourceLocation(rawLocation);
    }
    getScopeInfo(type) {
        return this.plugin.getScopeInfo(type);
    }
    listVariablesInScope(rawLocation) {
        return this.plugin.listVariablesInScope(rawLocation);
    }
    removeRawModule(rawModuleId) {
        return this.plugin.removeRawModule(rawModuleId);
    }
    getFunctionInfo(rawLocation) {
        return this.plugin.getFunctionInfo(rawLocation);
    }
    getInlinedFunctionRanges(rawLocation) {
        return this.plugin.getInlinedFunctionRanges(rawLocation);
    }
    getInlinedCalleesRanges(rawLocation) {
        return this.plugin.getInlinedCalleesRanges(rawLocation);
    }
    getMappedLines(rawModuleId, sourceFileURL) {
        return this.plugin.getMappedLines(rawModuleId, sourceFileURL);
    }
    async hello(moduleConfigurations, logPluginApiCalls) {
        this.#plugin = await createPlugin(this, this.resourceLoader, moduleConfigurations, logPluginApiCalls);
    }
}

// Copyright 2023 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
class ResourceLoader {
    async fetchSymbolsData(rawModule, url, hostInterface) {
        if (rawModule.code) {
            return { symbolsData: rawModule.code, symbolsDwpData: rawModule.dwp };
        }
        const symbolsResponse = await fetch(url.href, { mode: 'no-cors' });
        if (symbolsResponse.ok) {
            let symbolsDwpResponse = undefined;
            let symbolsDwpError;
            const dwpUrl = `${url.href}.dwp`;
            try {
                symbolsDwpResponse = await fetch(dwpUrl, { mode: 'no-cors' });
            }
            catch (e) {
                symbolsDwpError = e.message;
                // Unclear if this ever happens; usually if the file isn't there we
                // get a 404 response.
                console.error(symbolsDwpError);
            }
            if (!(symbolsDwpResponse && symbolsDwpResponse.ok)) {
                // Often this won't exist, but remember the missing file because if
                // we can't find symbol information later it is likely because this
                // file was missing.
                this.possiblyMissingSymbols = [`${url.pathname}.dwp`];
                if (symbolsDwpResponse) {
                    symbolsDwpError = symbolsDwpResponse?.statusText || `status code ${symbolsDwpResponse.status}`;
                }
            }
            const [symbolsData, symbolsDwpData] = await Promise.all([
                symbolsResponse.arrayBuffer(),
                symbolsDwpResponse && symbolsDwpResponse.ok ? symbolsDwpResponse.arrayBuffer() : undefined,
            ]);
            void hostInterface.reportResourceLoad(url.href, { success: true, size: symbolsData.byteLength });
            if (symbolsDwpData) {
                void hostInterface.reportResourceLoad(dwpUrl, { success: true, size: symbolsDwpData.byteLength });
            }
            else {
                void hostInterface.reportResourceLoad(dwpUrl, { success: false, errorMessage: `Failed to fetch dwp file: ${symbolsDwpError}` });
            }
            return { symbolsData, symbolsDwpData };
        }
        const statusText = symbolsResponse.statusText || `status code ${symbolsResponse.status}`;
        if (rawModule.url !== url.href) {
            const errorMessage = `NotFoundError: Unable to load debug symbols from '${url}' for the WebAssembly module '${rawModule.url}' (${statusText}), double-check the parameter to -gseparate-dwarf in your Emscripten link step`;
            void hostInterface.reportResourceLoad(url.href, { success: false, errorMessage });
            throw new Error(errorMessage);
        }
        const errorMessage = `NotFoundError: Unable to load debug symbols from '${url}' (${statusText})`;
        void hostInterface.reportResourceLoad(url.href, { success: false, errorMessage });
        throw new Error(errorMessage);
    }
    getModuleFileName(rawModuleId) {
        return `${self.btoa(rawModuleId)}.wasm`.replace(/\//g, '_');
    }
    async loadSymbols(rawModuleId, rawModule, symbolsURL, fileSystem, hostInterface) {
        const { symbolsData, symbolsDwpData } = await this.fetchSymbolsData(rawModule, symbolsURL, hostInterface);
        const symbolsFileName = this.getModuleFileName(rawModuleId);
        const symbolsDwpFileName = symbolsDwpData && `${symbolsFileName}.dwp`;
        // This file is sometimes preserved on reload, causing problems.
        try {
            fileSystem.unlink('/' + symbolsFileName);
        }
        catch {
        }
        fileSystem.createDataFile('/', symbolsFileName, new Uint8Array(symbolsData), true /* canRead */, false /* canWrite */, true /* canOwn */);
        if (symbolsDwpData && symbolsDwpFileName) {
            fileSystem.createDataFile('/', symbolsDwpFileName, new Uint8Array(symbolsDwpData), true /* canRead */, false /* canWrite */, true /* canOwn */);
        }
        return { symbolsFileName, symbolsDwpFileName };
    }
    createSymbolsBackendModulePromise() {
        const url = new URL('SymbolsBackend.wasm', import.meta.url);
        return fetch(url.href, { credentials: 'same-origin' }).then(response => {
            if (!response.ok) {
                throw new Error(response.statusText);
            }
            return WebAssembly.compileStreaming(response);
        });
    }
    possiblyMissingSymbols;
}

// Copyright 2023 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// eslint-disable-next-line @typescript-eslint/no-explicit-any
new RPCInterface(globalThis, new ResourceLoader());
