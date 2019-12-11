// Code generated by the Terumi Compiler, using the C target.
// The generated code is closer to `main`, at the end of the file.
// This code should be able to be compiled wherever you want. Clang is ideal.

#include <stdlib.h> // for 'malloc' and 'free'
#include <stddef.h> // explicit inclusion of 'size_t' and friends (imported by stdlib.h)
#include <stdint.h> // for 'int32_t' and friends
#include <stdio.h> // for 'printf'
#include <string.h> // for 'strcmp', 'strlen', etc.
#include <stdbool.h> // booleans

/*
NOTICE: partial amounts of this code was written without using size_t & (u)intN_t
if you wish to contribute, changing 'int's to their appropriate type is good.
- size_t: counts of something, length of something, or indexes
- uint32_t: everything else
- int32_t: when explicit negatives are needed (eg. Numbers)

The // <INJECT__CODE> comment signals to the C target to begin translating all
	of the user defined methods at that point.

The // <INJECT__RUN> comment signals to the C target to execute each viable
	main method at that point.
*/

/*
Debug mode enables a couple extra checks which will allow for program faults
to be found earlier, but at the cost of performance.
*/
#define DEBUG

/*
Tracing will print a visible trace of method call stacks for the developer, aka
the person debugging, to follow along. Not all methods are marked with trace
calls, so they must be created as necessary.
*/
// #define DEBUG_TRACING
#ifdef DEBUG_TRACING
static int level = 0;
void TRACE(const char* place) {
	for (int i = 0; i < level; i++) printf(" ");
	printf("[TRACE] at '%s'\n", place);
	level++;
}
void TRACE_EXIT(const char* place) {
	level--;
	for (int i = 0; i < level; i++) printf(" ");
	printf("[TRACE] left '%s'\n", place);
}
#define TRACE_EXIT_RETURN(data, place) TRACE_EXIT(place); return data;
#else
#define TRACE(place)
#define TRACE_EXIT(place)
#define TRACE_EXIT_RETURN(data, place)
#endif

// defines how many fields there should be for an object
// NOTICE: DO NOT CHANGE '3' - Terumi will replace '3' with it's own number.
#define GC_OBJECT_FIELDS 3

// if a list runs out of space, it will grow reallocate a new array and copy
// all old elements into the new list.
#define LIST_GROW_MULTIPLIER 3

// defines how big a chunk of memory should be initially.
#define DEFAULT_MEMORY_PAGE_SIZE 4000000

// Ensure exit codes are all uniform.
enum ExitCode {
	SUCCESS = 0,

	// The user's terumi code panics.
	PANIC = 1,

	// Typically used to refer to when a given operation is attempted to be
	// performed on a given Value, but the types end up being incompatible.
	// This is a result of a lack of strict enough type checking in the CTarget.
	TOO_DYNAMIC = 2,

	// Used when a given operation was attempted that ends up being impossible.
	// An example would be attempting to make a value copy with an UNKNOWN type.
	IMPOSSIBLE = 3,
};

// directly related to exit codes, prints additional information.
void print_err() {
	printf(
	//  "[ERR] ..."
	//   ^^^^^^ spacing to maintain symmetry
		"      you've just encountered a terumi error. There is something "
		"wrong with your code that Terumi isn't catching. Please report "
		"this bug to the terumi compiler repository, located at "
		"https://github.com/terumi-project/Terumi"
	);
}

// Helper for void pointer arrays to prevent screwups.
struct VoidPtrArray {
	void** data;
#ifdef DEBUG
	// in debug mode, there is bounds checking
	size_t length;
#endif
};

// Lists are regrowable arrays. If an element is attempted to be added and the
// list is full, a new array will be allocated LIST_GROW_MULTIPLIER times the
// size, and all old data will be copied.
struct List {
	// The inner array of data, represented using a VoidPtrArray
	struct VoidPtrArray array;

	// The total size of the array.
	size_t capacity;

	// The total amount of elements added. This number will constantly be
	// reaching 'capacity', and a fter that, the list will resize.
	size_t elements;
};

// Metadata for the type of an object in Terumi.
enum ObjectType {

	// 'Unknown' is primarily used to represent newly allocated objects.
	UNKNOWN = 0,

	// 'String' is used to represent objects whose data contains a char*
	STRING = 1,

	// 'Number' is used to represent objects whose data contains a 'struct Number*'
	NUMBER = 2,

	// bool*
	BOOLEAN = 3,

	// struct Object*
	OBJECT = 4,
};

const char* objecttype_to_string(enum ObjectType type) {
	switch (type) {
		case UNKNOWN: return "UNKNOWN";
		case STRING: return "STRING";
		case NUMBER: return "NUMBER";
		case BOOLEAN: return "BOOLEAN";
		case OBJECT: return "OBJECT";
		default: return "N/A";
	}
}

// A 'Value' is used to represent a given value in Terumi. Because Terumi is a
// dynamic language, 'Value' has a generic void* to data, with an 'ObjectType'
// to serve as a signifier for what kind of data is stored. 'Value's are freed
// by the Terumi GC. Do not free them.
struct Value {
	// The type of the value. Useful for determining if it was freshly
	// allocated, or if it is compatible with a call to a given function.
	enum ObjectType type;

	// A pointer to the data of this function. A value must guarentee these
	// kinds of values depending upon the type:
	// UNKNOWN - uninitialized data
	// STRING - char* pointer
	// NUMBER - struct Number* pointer
	// BOOLEAN - bool* pointer
	// OBJECT - struct Object* pointer
	void* data;
};

// A 'ValuePtr' is an index in the GC to a GCEntry, which contains a Value.
// These are used instead of pointers to Value. The GC, on collection, will
// update all 'Value's that are 'Object's with "old value pointers". These
// are particularly useful to gain immediate access to the GCEntry of a value
// in the GC.
struct ValuePtr {
	int32_t object_index;
};

// An 'Object' is used to represent a series of fields, with values. This is
// represented with pointers to values.
struct Object {
	// An array of ValuePtrs to values used by a given object.
	struct ValuePtr fields[GC_OBJECT_FIELDS];
};

// Terumi's number system may expand in the future to floating point operations
// or more. 'Number' is used to encapsulate this. Never access 'data', instead,
// use or create Number specific functions.
struct Number {
	int32_t data;
};

// Caches multiple Values that are typically used everywhere. This currently
// only caches the booleans TRUE, FALSE, and the numbers -1, 0, 1, and 2.
struct ValueCache {
	bool* true_ptr;
	bool* false_ptr;
};

// An instance of the GC. Maintains all GC-related data.
struct GC {
	// A list of struct GCEntry*. All objects that are handheld with gc_handhold
	// are added here.
	struct List list;

	// The threshold of the GC, which is arbitrarily used to determine when to
	// collect.
	size_t threshold;

	// A cache of commonly used values to prevent reallocation and GC pressure.
	struct ValueCache cache;
};

// A GCEntry manages holding a given Value.
struct GCEntry {
	// 'active' only specifies if this object is out of scope of the method it
	// was created in. Once it's not active, it is only kept alive by
	// references to it.
	bool active;

	// 'alive' only specifies if the GCEntry is alive - used as a 'tombstone'.
	// This is particularly useful for the sweep & compact phase of the GC.
	bool alive;

	// A pointer to the value this GCEntry holds.
	struct Value* value;

	// The index of this GCEntry in the GC. Useful for 'ValuePtr's.
	size_t index;
};

// An instance of Memory. Maintains all Memory-related data.
struct Memory {
	// The size of a page. Will increase if lots of data is needed.
	size_t page_size;

	// A list of struct Memory*. This maintains all the memory chunks.
	struct List memory_pages;
};

// A large block of memory, which terumi_alloc and terumi_free use.
struct MemoryPage {
	// The size of this memory page.
	size_t size;

	// A raw pointer to the data of this memory page.
	void* data;

	// How many bytes of the memory page has been used. Adding data to used
	// will result in a pointer to where unclaimed data is.
	size_t used;

	// The amount of renters. Every time a consumer allocates some memory, an
	// appropriate memory block is found and the rented number is incremented.
	// Once the user returns that block, the rented number is decremented, and
	// if this block has no renters, 'used' will be reset upon the next alloc.
	uint32_t rented;
};

// Creates a new void pointer array with a given size.
struct VoidPtrArray* voidptrarray_new(size_t capacity);
// Resizes a void pointer array to a new size. The old size is used in debug
// mode for array bound checks.
void voidptrarray_regrow(struct VoidPtrArray* data, size_t new_size, size_t old_size);
// Gets an element at a given index of a struct VoidPtrArray*.
void* voidptrarray_at(struct VoidPtrArray* array, size_t index);
// Sets an element at a given index of a struct VoidPtrArray*.
void voidptrarray_set(struct VoidPtrArray* array, size_t index, void* value);

// Constructs a new List with an initial capacity.
struct List* list_new(size_t initial_capacity);
// Appends an item to a given list, and resizes if necessary.
size_t list_add(struct List* list, void* item);

// Creates a value with an uninitialized pointer.
struct Value* value_blank(enum ObjectType type);
// Constructs a Value from a string. Uses the terumi allocator.
struct Value* value_from_string(char* string);
// Constructs a Value from a boolean. Reuses a global Value*. Do not mark as
// inactive.
struct Value* value_from_boolean(bool state);
// Constructs a Value from a Number. Uses the terumi allocator. Do not free
// the number after passing it in, that is for the Value* to manage. Do not
// pass in Numbers not created with the number_from_x functions.
struct Value* value_from_number(struct Number* number);
// Constructs a Value from an Object. Uses the terumi allocator.
struct Value* value_from_object(struct Object* object);
// Makes a complete deep copy of the inner value of a give nvalue.
void* value_copy(struct Value* source);
char* value_unpack_string(struct Value* value);
bool value_unpack_boolean(struct Value* value);
struct Number* value_unpack_number(struct Value* value);
struct Object* value_unpack_object(struct Value* value);

struct Object* object_blank();

// Constructs a number from an integer.
struct Number* number_from_int(int integer);
struct Number* number_add(struct Number* left, struct Number* right);
struct Number* number_subtract(struct Number* left, struct Number* right);
struct Number* number_multiply(struct Number* left, struct Number* right);
struct Number* number_divide(struct Number* left, struct Number* right);
struct Number* number_power(struct Number* base, struct Number* power);
bool number_equal(struct Number* left, struct Number* right);

// Global instance of the GC. Used for managing 'struct Value's.
struct GC gc;
bool gc_init = false;

// Ensures the GC is initialized.
void ensure_gc_init();
// Runs the GC. Returns the amount of objects collected.
size_t run_gc();
// Returns 'true' if the GC should be ran (some threshold was passed)
bool should_run_gc();
// Might run the GC. Returns -1 if it didn't, otherwise it returns the amount
// of objects the GC collected.
errno_t maybe_run_gc();
// registers a Value* to be managed (aka, handheld) by the GC. The caller is
// expected to set 'active' to 'false' on the returned GC entry once they are
// no longer going to use the value. Values must be created with the value
// allocator functions. The caller should NOT call terumi_free, as the GC will.
struct GCEntry* gc_handhold(struct Value* value);

// Global instance of the Memory. Used for the terumi allocators.
struct Memory memory;
bool memory_init = false;

// Ensures the Memory is initialized.
void ensure_memory_init();
// Allocates some data with terumi's memory paging system.
void* terumi_alloc(size_t data);
// Frees some data with terumi's memory paging system.
bool terumi_free(void* data);
// If 'terumi_free' fails, this will print a message to standard out for you.
void terumi_free_warn(void* data);

// Uses the Terumi GC to allocate a copy of a string.
char* string_copy(char* source);

#pragma region VoidPtrArray
struct VoidPtrArray* voidptrarray_new(size_t capacity) {
	TRACE("voidptrarray_new");
	struct VoidPtrArray* array = malloc(sizeof(struct VoidPtrArray));
	void** data = malloc(sizeof(void*) * capacity);
	array->data = data;
#ifdef DEBUG
	array->length = capacity;
#endif
	TRACE_EXIT("voidptrarray_new");
	return array;
}

void voidptrarray_regrow(struct VoidPtrArray* data, size_t new_size, size_t old_size) {
	TRACE("voidptrarray_regrow");
	if (data->data == NULL) {
		printf("data NULL");
	}

	// TODO: use realloc
	void** new = malloc(sizeof(void*) * new_size);
	// memcpy doesn't seem to work...?
	for (int i = 0; i < old_size; i++) {
		new[i] = data->data[i];
	}
	free(data->data);
	data->data = new;
#ifdef DEBUG
	data->length = new_size;
#endif

	TRACE_EXIT("voidptrarray_regrow");
}

void* voidptrarray_at(struct VoidPtrArray* data, size_t index) {
#ifdef DEBUG
	if (index >= data->length) {
		printf("[WARN] OUT OF BOUNDS ACCESS ON VOID POINTER ARRAY: accesing '%zu' of length '%zu'\n", index, data->length);
	}
#endif
	void* voidptr = data->data[index];
	return voidptr;
}

void voidptrarray_set(struct VoidPtrArray* data, size_t index, void* value) {
#ifdef DEBUG
	if (index >= data->length) {
		printf("[WARN] OUT OF BOUNDS ACCESS ON VOID POINTER ARRAY: accesing '%zu' of length '%zu'\n", index, data->length);
	}
#endif
	data->data[index] = value;
}
#pragma endregion

#pragma region List

size_t list_add(struct List* list, void* item) {
	// if the list has reached maximum capacity
	if (list->elements >= list->capacity) {

		// double the capacity
		size_t new_capacity = list->capacity * LIST_GROW_MULTIPLIER;
		voidptrarray_regrow(&list->array, new_capacity, list->capacity);
		list->capacity = new_capacity;
	}

	// insert the item into the array
	size_t index = list->elements++;
	voidptrarray_set(&list->array, index, item);
	return index;
}

// the memory paging isn't setup and needs a list
// so it calls the malloc version
struct List* list_new(size_t initial_capacity) {
	TRACE("list_new_malloc");
	struct List* list = malloc(sizeof(struct List));
	list->capacity = initial_capacity;
	list->elements = 0;
	list->array = *voidptrarray_new(initial_capacity);
	TRACE_EXIT("list_new_malloc");
	return list;
}

#pragma endregion

#pragma region Value
void ensure_type(struct Value* value, enum ObjectType must_be) {
	if (value->type != must_be) {
		printf(
			"[ERR] couldn't ensure value of type '%s', value was type '%s'\n",
			objecttype_to_string(value->type),
			objecttype_to_string(must_be)
		);

		print_err();
		exit(TOO_DYNAMIC);
	}
}

void ensure_not_type(struct Value* value, enum ObjectType mustnt_be) {
	if (value->type == mustnt_be) {
		printf(
			"[ERR] value of type '%s', isn't allowed. type musn't be '%s'\n",
			objecttype_to_string(value->type),
			objecttype_to_string(mustnt_be)
		);

		print_err();
		exit(TOO_DYNAMIC);
	}
}

struct Value* value_blank(enum ObjectType type) {
	struct Value* value = terumi_alloc(sizeof(struct Value));
	value->type = type;
	return value;
}

struct Value* value_from_string(char* string) {
	struct Value* value = value_blank(STRING);
	value->data = string;
	return value;
}

struct Value* value_from_boolean(bool state) {
	ensure_gc_init();
	struct Value* v = value_blank(BOOLEAN);

	if (state) {
		v->data = gc.cache.true_ptr;
	} else {
		v->data = gc.cache.false_ptr;
	}

	return v;
}

struct Value* value_from_number(struct Number* number) {
	struct Value* value = value_blank(NUMBER);
	value->data = number;
	return value;
}

struct Value* value_from_object(struct Object* object) {
	struct Value* value = value_blank(OBJECT);
	value->data = object;
	return value;
}

void* value_copy(struct Value* source) {
	ensure_gc_init();

	switch (source->type) {
		case BOOLEAN: return source->data;
		// these will automatically return cached values if necessary
		case NUMBER: return number_from_int(value_unpack_number(source)->data);
		// make a copy of the string
		case STRING: return string_copy(value_unpack_string(source));
		// makes a copy of the object
		case OBJECT: {
			struct Object* source_object = source->data;
			struct Object* copy = object_blank();

			for (size_t i = 0; i < GC_OBJECT_FIELDS; i++) {
				copy->fields[i].object_index = source_object->fields[i].object_index;
			}

			return copy;
		}

		default: break;
	}

	printf("[ERR] unable to make a copy of value with type '%s'.", objecttype_to_string(source->type));
	print_err();
	exit(IMPOSSIBLE);
}

char* value_unpack_string(struct Value* value) {
	ensure_type(value, STRING);
	return value->data;
}

bool value_unpack_boolean(struct Value* value) {
	ensure_type(value, BOOLEAN);
	return *((bool*)(value->data));
}

struct Number* value_unpack_number(struct Value* value) {
	ensure_type(value, NUMBER);
	return value->data;
}

struct Object* value_unpack_object(struct Value* value) {
	ensure_type(value, OBJECT);
	return value->data;
}
#pragma endregion

#pragma region GC
void ensure_gc_init() {
	if (!gc_init) {
		gc.list = *list_new(4096000);
		gc.threshold = 4096;

		bool* ptrtrue = terumi_alloc(sizeof(bool));
		*ptrtrue = true;

		bool* ptrfalse = terumi_alloc(sizeof(bool));
		*ptrfalse = false;

		// assigning values

		gc.cache = (struct ValueCache){
			.true_ptr = ptrtrue,
			.false_ptr = ptrfalse,
		};

		gc_init = true;
	}
}

errno_t maybe_run_gc() {
	ensure_gc_init();
	errno_t result = should_run_gc() ? run_gc() : -1;

	// GC NOTICE:
	// if we remove more than 60% of the objects, we will downsize our threshold by half
	// if we don't remove     ^^^ of the objects, we will increase our threshold by 2x

	float percent_60 = (float)result / (float)gc.threshold;

	if (percent_60 > 0.60) {
		// gc.threshold /= 2;
	}
	else {
		// gc.threshold *= 2;
	}

	return result;
}

// we want to completely nuke this value from orbit
void cleanup_value(struct Value* value) {
	TRACE("cleanup_value");

	switch (value->type) {
		case UNKNOWN: {
			printf("[WARN] attempting to free UNKNOWN value. please report this bug.");
		} break;
		case BOOLEAN: {
			// don't free the pointer to the data for a boolean
			terumi_free(value);
		} break;
		case STRING: {
			// all strings should be allocated with terumi
			terumi_free(value->data);
			terumi_free(value);
		} break;
		case NUMBER: {
			// free the Number & value
			terumi_free(value->data);
			terumi_free(value);
		} break;
		case OBJECT: {
			terumi_free(value->data);
			terumi_free(value);
		} break;
	}
	TRACE_EXIT("cleanup_value");
}

bool should_run_gc() {
	ensure_gc_init();
	bool should = gc.list.elements >= gc.threshold;
	return should;
}

bool mark_referenced(bool* referenced) {
	TRACE("mark_referenced");
	bool modified = false;

	for (size_t i = 0; i < gc.list.elements; i++) {
		struct GCEntry* gcentry = voidptrarray_at(&gc.list.array, i);

		// active items are referenced in a method
		if (gcentry->active
			// that aren't already marked
			&& !referenced[i]) {

			referenced[i] = true;
			modified = true;
		}

		// if it's not referenced, we don't want to mark anything it references
		// as referenceable.
		if (!referenced[i]) continue;

		// has to be an object so we can mark stuff as referenceable
		struct Value* value = gcentry->value;
		if (value->type != OBJECT) continue;

		struct Object* object = value->data;

		// mark each object the referenced object references as referenced.
		for (size_t j = 0; j < GC_OBJECT_FIELDS; j++) {

			size_t index = object->fields[j].object_index;

			if (!referenced[index]) {
				modified = true;
				referenced[index] = true;
			}
		}
	}

	TRACE_EXIT("mark_referenced");
	return modified;
}

void swap_gc(size_t from, size_t to) {
	TRACE("swap_gc");

	// move the item from 'from' to 'to'
	struct GCEntry* at_from = voidptrarray_at(&gc.list.array, from);
	at_from->index = to;

	struct GCEntry* at_to = voidptrarray_at(&gc.list.array, to);
	at_to->index = from;

	voidptrarray_set(&gc.list.array, to, at_from);
	voidptrarray_set(&gc.list.array, from, at_to);

	// update all references in the GC
	for (size_t i = 0; i < gc.list.elements; i++) {
		struct GCEntry* datai = voidptrarray_at(&gc.list.array, i);

		if (!(datai->alive)) continue;
		if (datai->value->type != OBJECT) continue;

		struct Object* obj = datai->value->data;
		for (size_t j = 0; j < GC_OBJECT_FIELDS; j++) {
			int32_t index = obj->fields[j].object_index;

			if (index == from) {
				obj->fields[j].object_index = to;
			} else if (index == to) {
				obj->fields[j].object_index = from;
			}
		}
	}
	TRACE_EXIT("swap_gc");
}

void compact_gc() {
	TRACE("compact_gc");
	int begin = 0;
	int end = gc.list.elements - 1;
	bool was_swap = false;

	// to compact the GC, we'll swap alive items from the back to dead spaces
	// in the front.
	while (true) {

		// find a dead item in the front
		for (; begin < gc.list.elements; begin++) {
			struct GCEntry* entry = voidptrarray_at(&gc.list.array, begin);

			if (!(entry->alive)) {
				break;
			}
		}

		// find an alive item from the back
		for (; end >= 0; end--) {
			struct GCEntry* entry = voidptrarray_at(&gc.list.array, end);

			if (entry->alive) {
				break;
			}
		}

		if (end == -1) {
			// there isn't a single alive item
			break;
		}

		// if the begin is in front of the end, don't swap
		if (begin >= end) break;

		swap_gc(end, begin);
		was_swap = true;
	}

	// nothing is to be compacted
	if (!was_swap) {
		TRACE_EXIT("compact_gc");
		return;
	}

	// find the first dead element from the beginning
	size_t dead_index = 0;
	for (size_t i = 0; i < gc.list.elements; i++) {
		struct GCEntry* entry = voidptrarray_at(&gc.list.array, i);

		if (entry->alive) continue;
		dead_index = i;
		break;
	}

	// now that dead index is the index of the first dead item,
	// free everything starting at it up until the end

	for (size_t i = dead_index; i < gc.list.elements; i++) {
		struct GCEntry* entry = voidptrarray_at(&gc.list.array, i);
		terumi_free(entry);
	}

	gc.list.elements = dead_index;

	TRACE_EXIT("compact_gc");
}

size_t run_gc() {
	TRACE("run_gc");
	ensure_gc_init();
	// MARK
	bool* referenced = terumi_alloc(sizeof(bool) * gc.list.elements);
	memset(referenced, false, sizeof(bool) * gc.list.elements);

	// this will mark all objects referenced by an alive object as alive.
	while (mark_referenced(referenced)) {
	}

	// SWEEP
	size_t cleaned = 0;
	for (size_t i = 0; i < gc.list.elements; i++) {
		if (!referenced[i]) {
			struct GCEntry* entry = voidptrarray_at(&gc.list.array, i);

			if (entry->alive) {
				cleaned++;
				entry->active = false;
				entry->alive = false;
				cleanup_value(entry->value);
				// don't clean up 'entry' here - that's cleaned up in compacting
			}
		}
	}

	if (cleaned > 0) {
		compact_gc();
	}

#ifdef DEBUG
	printf("[GC] cleaned %zu objects.\n", cleaned);
#endif

	terumi_free_warn(referenced);
	TRACE_EXIT("run_gc");
	return cleaned;
}

struct GCEntry* gc_handhold(struct Value* value) {
	ensure_gc_init();

	struct GCEntry* entry = terumi_alloc(sizeof(struct GCEntry));
	entry->alive = true;
	entry->active = true;
	entry->value = value;
	entry->index = list_add(&gc.list, entry);
	maybe_run_gc();
	return entry;
}
#pragma endregion

#pragma region Memory
/* Memory Paging */
// memory paging creates pages of memory and allocates and deallocates from the
// pages, instead of individual malloc calls. That way, fewer mallocs are
// performed.
size_t terumi_resize_page_size(size_t initial, size_t target) {
	TRACE("terumi_resize_page_size");
	size_t size = initial;

	while (size < target) {
		size *= 2;
	}

	TRACE_EXIT("terumi_resize_page_size");
	return size;
}

// News up a page - used and rented are left uninitialized for the caller to
// set.
struct MemoryPage* terumi_new_page(size_t page_size) {
	TRACE("terumi_new_page");
	void* data = malloc(page_size);
	struct MemoryPage* page = malloc(sizeof(struct MemoryPage));
	page->size = page_size;
	page->data = data;
	// page->used = 0;
	// page->rented = 0;
	list_add(&memory.memory_pages, page);
	TRACE_EXIT("terumi_new_page");
	return page;
}

void ensure_memory_init() {
	if (!memory_init) {
		memory.memory_pages = *list_new(4);
		memory.page_size = DEFAULT_MEMORY_PAGE_SIZE;
		memory_init = true;
	}
}

// in debug mode, we have a one byte preamble to ensure that we never free the
// same memory twice. in release mode, we don't have this.

void* terumi_alloc_raw(size_t data) {
	ensure_memory_init();

	if (memory.page_size < data) {
		// if the data is bigger than a page, we are guarenteed to not have any
		// pages open. resize the page, then allocate a new page.

		memory.page_size = terumi_resize_page_size(memory.page_size, data);
		struct MemoryPage* page = terumi_new_page(memory.page_size);

		page->rented = 1;
		page->used = data;
		void* page_data = page->data;
		return page_data;
	}

	// look through all available pages, and see if there's enough to rent for
	// the caller
	for (int i = 0; i < memory.memory_pages.elements; i++) {
		struct MemoryPage* page = voidptrarray_at(&memory.memory_pages.array, i);

		if (page->rented == 0) {
			// if this is an empty page, we can allocate here.
			page->used = data;
			page->rented = 1;
			void* page_data = page->data;
			return page_data;
		}

		if (data + page->used < page->size) {
			// there's enough space in this page to allocate here
			void* voidptr = page->data + page->used;
			page->used += data;
			page->rented++;
			return voidptr;
		}
	}

	// there are no pages big enough for the caller. we'll make a new page.
	for (int i = 0; i < memory.memory_pages.elements; i++) {
		struct MemoryPage* page = voidptrarray_at(&memory.memory_pages.array, i);
	}

	struct MemoryPage* page = terumi_new_page(memory.page_size);
	page->used = data;
	page->rented = 1;
	void* page_data = page->data;
	return page_data;
}

bool terumi_free_raw(void* data) {

	if (data == NULL) {
		return true;
	}

	ensure_memory_init();

	// first, let's find the page the pointer is in
	for (int i = 0; i < memory.memory_pages.elements; i++) {
		struct MemoryPage* page = voidptrarray_at(&memory.memory_pages.array, i);

		// if the pointer is within the block of memory
		if ((data >= page->data) && (data < (page->data + page->size))) {

			// there's one less renter.
			page->rented--;
#ifdef DEBUG
			if (page->rented < 0) {
				printf("[WARN] page->rented < 0");
			}
#endif
			return true;
		}
	}

	return false;
}

#ifdef DEBUG
void* terumi_alloc(size_t data) {
	// allocate one extra byte to signify if this data has been freeed
	void* mem = terumi_alloc_raw(data + 1);
	int8_t* mem_preamble = mem;
	*mem_preamble = (int8_t)1 /*true*/;
	return mem + 1;
}

bool terumi_free(void* data) {
	void* mem = data;
	int8_t* mem_preamble = data - 1;

	if (!mem_preamble[0]) {
		printf("[WARN] attempting to free a pointer to data that is already freed.\n");
		return true;
	}

	mem_preamble[0] = (int8_t)0;
	terumi_free_raw(mem);
	return true;
}
#else
void* terumi_alloc(size_t data) {
	return terumi_alloc(data);
}

bool terumi_free(void* data) {
	return terumi_free(data);
}
#endif

void terumi_free_warn(void* x) {
	TRACE("terumi_free_warn");
	if (!terumi_free(x)) {
		//printf("[WARN] Unable to free data at '%i'\n", (int)x);
	}
	TRACE_EXIT("terumi_free_warn");
}
#pragma endregion

#pragma region Number
struct Number* number_from_int(int32_t integer) {
	struct Number* number = terumi_alloc(sizeof(struct Number));
	number->data = integer;
	return number;
}

struct Number* number_add(struct Number* left, struct Number* right) {
	return number_from_int(left->data + right->data);
}

struct Number* number_subtract(struct Number* left, struct Number* right) {
	return number_from_int(left->data - right->data);
}

struct Number* number_multiply(struct Number* left, struct Number* right) {
	return number_from_int(left->data * right->data);
}

struct Number* number_divide(struct Number* left, struct Number* right) {
	return number_from_int(left->data / right->data);
}

// TODO: find out function
struct Number* number_power(struct Number* base, struct Number* power) {
	printf("[WARN] using powers - not supported yet.");
	return number_from_int(base->data + power->data);
}

bool number_equal(struct Number* left, struct Number* right) {
	return left->data == right->data;
}
#pragma endregion

struct Object* object_blank() {
	struct Object* object = terumi_alloc(sizeof(struct Object));

	// TODO: might be able to use memset
	for (size_t i = 0; i < GC_OBJECT_FIELDS; i++) {
		object->fields[i].object_index = -1;
	}

	return object;
}

char* string_concat(char* a, char* b) {
	ensure_gc_init();

	size_t strlen_a = strlen(a);
	size_t strlen_b = strlen(b);
	size_t str_len = strlen_a + strlen_b + sizeof(char);
	char* dst = terumi_alloc(str_len);
	memcpy(dst, a, strlen_a);
	memcpy(dst + strlen_a, b, strlen_b);
	dst[strlen_a + strlen_b] = '\0';

	return dst;
}

char* string_copy(char* source) {
	ensure_gc_init();

	size_t str_length = strlen(source) + sizeof(char);
	char* dst = terumi_alloc(str_length);
	strncpy_s(dst, str_length, source, str_length);
	return dst;
}

#pragma region Instructions
/* This is a slew of instruction specific methods implemented by terumi. */
struct Value* instruction_load_string(const char* data);
struct Value* instruction_load_number(int32_t integer);
struct Value* instruction_load_boolean(bool boolean);
struct GCEntry* instruction_load_parameter(struct GCEntry* parameters, size_t parameter_index);
void instruction_assign(struct Value* target, struct Value* source);
void instruction_set_field(struct GCEntry* value, struct Value* object, size_t field_index);
struct GCEntry* instruction_get_field(struct Value* object, size_t field_index);
struct Value* instruction_new();

struct Value* instruction_load_string(const char* data) {
	return value_from_string(string_copy(data));
}

struct Value* instruction_load_number(int32_t integer) {
	return value_from_number(number_from_int(integer));
}

struct Value* instruction_load_boolean(bool boolean) {
	return value_from_boolean(boolean);
}

struct GCEntry* instruction_load_parameter(struct GCEntry* parameters, size_t parameter_index) {
	return &(parameters[parameter_index]);
}

void instruction_assign(struct Value* target, struct Value* source) {
	ensure_not_type(source, UNKNOWN);

	if (target->type == UNKNOWN) {
		target->type = source->type;

		// no need to free target data, there is nothing
		target->data = value_copy(source);
		return;
	}

	if (target->type == source->type) {
		// assigning two booleans doesn't need anything to be freed
		if (source->type == BOOLEAN) {

			if (value_unpack_boolean(source) == true) {
				target->data = gc.cache.true_ptr;
			} else {
				target->data = gc.cache.false_ptr;
			}

			return;
		}

		terumi_free(target->data);
		target->data = value_copy(source);
		return;
	}

	switch (target->type) {
		case STRING: {
			switch (source->type) {
				// number -> string
				case NUMBER: {
					// 12 should be all we need, but we allocate 3x as much just in case :^)
					const char str_buffer[12 * 3];
					size_t wrote = sprintf(str_buffer, "%d", ((struct Number*)(source->data))->data);
					// we copy the buffer into a string
					// sprintf should write a null terminator for us
					char* str = string_copy(str_buffer);
					target->data = str;
				} return;
				default: break;
			}
		} break;
		case NUMBER: {
			switch (source->type) {
				// string -> number
				case STRING: {
					long num_val = strtol(source->data, source->data + strlen(source->data), 10);

					if (errno != 0) {
						printf(
							"[ERR] got error '%s' while attempting to convert string '%s' into a number.",
							strerror(errno),
							source->data
						);
						print_err();
						exit(IMPOSSIBLE);
					}

					if (num_val > INT32_MAX || num_val < INT32_MIN) {
						// couldn't convert
						printf(
							"[ERR] unable to assign type '%s' to '%s' - the converted string is too big (%ld).",
							objecttype_to_string(source->type),
							objecttype_to_string(target->type),
							num_val
						);
						print_err();
						exit(IMPOSSIBLE);
					}
				} return;
				default: break;
			}
		} break;
		case BOOLEAN: {
		} break;
		case OBJECT: {
		} break;
		default: break;
	}

	printf("[ERR] unable to assign type '%s' to '%s'.", objecttype_to_string(source->type), objecttype_to_string(target->type));
	print_err();
	exit(IMPOSSIBLE);
}

void instruction_set_field(struct GCEntry* value, struct Value* object, size_t field_index) {
	ensure_type(object, OBJECT);
	struct Object* data = object->data;
	data->fields[field_index].object_index = value->index;
}

struct GCEntry* instruction_get_field(struct Value* object, size_t field_index) {
	ensure_type(object, OBJECT);
	struct Object* data = object->data;
	int32_t object_index = data->fields[field_index].object_index;

	if (object_index == -1) {
		printf("[ERR] unable to get field '%i' from object when field isn't initialized.\n");
		print_err();
		exit(IMPOSSIBLE);
	}

	struct GCEntry* entry = voidptrarray_at(&gc.list.array, object_index);
	return entry;
}

struct Value* instruction_new() {
	maybe_run_gc();
	return value_from_object(object_blank());
}

#pragma endregion

#pragma region CompilerMethods
/* This is a slew of methods that can be called by the user, which are implemented. */
struct Value* cc_target_name();
// fake return type. the caller will not get input back.
struct Value* cc_panic(struct Value* message);
struct Value* cc_is_supported(struct Value* message);
void cc_println(struct Value* input);
void cc_command(struct Value* command);
struct Value* cc_operator_and(struct Value* left, struct Value* right);
struct Value* cc_operator_or(struct Value* left, struct Value* right);
struct Value* cc_operator_not(struct Value* operand);
struct Value* cc_operator_equal_to(struct Value* left, struct Value* right);
struct Value* cc_operator_not_equal_to(struct Value* left, struct Value* right);
struct Value* cc_operator_less_than(struct Value* left, struct Value* right);
struct Value* cc_operator_greater_than(struct Value* left, struct Value* right);
struct Value* cc_operator_less_than_or_equal_to(struct Value* left, struct Value* right);
struct Value* cc_operator_greater_than_or_equal_to(struct Value* left, struct Value* right);
struct Value* cc_operator_add(struct Value* left, struct Value* right);
struct Value* cc_operator_negate(struct Value* operand);
struct Value* cc_operator_subtract(struct Value* left, struct Value* right);
struct Value* cc_operator_multiply(struct Value* left, struct Value* right);
struct Value* cc_operator_divide(struct Value* left, struct Value* right);
struct Value* cc_operator_exponent(struct Value* left, struct Value* right);

struct Value* cc_target_name() {
	return value_from_string(string_copy("c"));
}

struct Value* cc_panic(struct Value* message) {
	ensure_type(message, STRING);

	printf("[PANIC] panic in user code: '%s'\n", (char*)(message->data));
	exit(PANIC);

	return NULL;
}

struct Value* cc_is_supported(struct Value* message) {
	ensure_type(message, STRING);
	char* data = message->data;
	
	// TODO: check stuff
	return value_from_boolean(true);
	// if (strcmp("") != 0)
}

void cc_println(struct Value* input) {
	ensure_type(input, STRING);
	printf("%s\n", (char*)(input->data));
}

void cc_command(struct Value* command) {
	system(value_unpack_string(command));
}

struct Value* cc_operator_and(struct Value* left, struct Value* right) {
	return value_from_boolean(value_unpack_boolean(left) && value_unpack_boolean(right));
}

struct Value* cc_operator_or(struct Value* left, struct Value* right) {
	return value_from_boolean(value_unpack_boolean(left) | value_unpack_boolean(right));
}

struct Value* cc_operator_not(struct Value* operand) {
	return value_from_boolean(!value_unpack_boolean(operand));
}

struct Value* cc_operator_equal_to(struct Value* left, struct Value* right) {
	// equal to must be same types
	ensure_type(right, left->type);

	switch (left->type) {
		case UNKNOWN: return value_from_boolean(true);
		case STRING: return value_from_boolean(strcmp(value_unpack_string(left), value_unpack_string(right)) != 0);
		case NUMBER: return value_from_boolean(number_equal(value_unpack_number(left), value_unpack_number(right)));
		case OBJECT: return value_from_boolean(left->data == right->data); // reference equality for objects
		case BOOLEAN: return value_from_boolean(value_unpack_boolean(left) == value_unpack_boolean(right));
	}

	printf("[ERR] unable to compare types '%s' and '%s'.", objecttype_to_string(left->type), objecttype_to_string(right->type));
	print_err();
	exit(IMPOSSIBLE);
	return NULL;
}

struct Value* cc_operator_not_equal_to(struct Value* left, struct Value* right) {
	return cc_operator_not(cc_operator_equal_to(left, right));
}

struct Value* cc_operator_less_than(struct Value* left, struct Value* right) {
	return value_from_boolean(value_unpack_number(left) < value_unpack_number(right));
}

struct Value* cc_operator_greater_than(struct Value* left, struct Value* right) {
	return value_from_boolean(value_unpack_number(left) > value_unpack_number(right));
}

struct Value* cc_operator_less_than_or_equal_to(struct Value* left, struct Value* right) {
	return cc_operator_not(cc_operator_greater_than(left, right));
}

struct Value* cc_operator_greater_than_or_equal_to(struct Value* left, struct Value* right) {
	return cc_operator_not(cc_operator_less_than(left, right));
}

struct Value* cc_operator_add(struct Value* left, struct Value* right) {
	if (left->type == STRING) {
		ensure_type(right, STRING);

		// concatenation
		return value_from_string(string_concat(value_unpack_string(left), value_unpack_string(right)));
	} else {

		// actual addition
		return value_from_number(number_add(value_unpack_number(left), value_unpack_number(right)));
	}
}

struct Value* cc_operator_negate(struct Value* operand) {
	return value_from_number(number_subtract(number_from_int(0), value_unpack_number(operand)));
}

struct Value* cc_operator_subtract(struct Value* left, struct Value* right) {
	return value_from_number(number_subtract(value_unpack_number(left), value_unpack_number(right)));
}

struct Value* cc_operator_multiply(struct Value* left, struct Value* right) {
	return value_from_number(number_multiply(value_unpack_number(left), value_unpack_number(right)));
}

struct Value* cc_operator_divide(struct Value* left, struct Value* right) {
	return value_from_number(number_divide(value_unpack_number(left), value_unpack_number(right)));
}

struct Value* cc_operator_exponent(struct Value* left, struct Value* right) {
	return value_from_number(number_power(value_unpack_number(left), value_unpack_number(right)));
}

#pragma endregion

#pragma region TerumiMethods
// <INJECT__CODE>
#pragma endregion

int main(int argc, char** argv) {
	TRACE("main");

	// must be on the first level of indentation
// <INJECT__RUN>

	// cleanup all GC resources
	for (int i = 0; i < gc.list.elements; i++) {
		// mark every entry as dead that isn't dead
		struct GCEntry* entry = voidptrarray_at(&gc.list.array, i);
		entry->active = false;
	}

	size_t cleared = run_gc();
#ifdef DEBUG
	printf("[GC] program ended: collected %zu objects. %zu objects alive.", cleared, gc.list.elements);
#endif

	// free all pages
	for (int i = 0; i < memory.memory_pages.elements; i++) {
		free(voidptrarray_at(&memory.memory_pages.array, i));
	}

	TRACE_EXIT("main");
	return 0;
}