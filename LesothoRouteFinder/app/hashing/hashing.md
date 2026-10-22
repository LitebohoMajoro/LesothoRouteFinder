# Task: Add Configurable Hash Table Sizes and Separate Text-File Representations

Modify my **existing C++ Win32 Hashing Visualizer** to support user-defined hash table sizes for both Open Hashing (Separate Chaining) and Closed Hashing (Linear Probing), and maintain a separate, properly formatted text file for each hash table.

**Important:** Work with the existing project. Inspect the current implementation before making changes. Do not rebuild the application from scratch or replace working functionality unnecessarily.

## 1. Allow the User to Choose the Hash Table Size

The user must be able to determine the initial hash table capacity independently for each hashing method.

### Open Hashing
- Add an option to configure the number of buckets used by `HashSetOpen`.
- Let the user enter a desired table size before creating or initializing the table.
- Validate the input. Reject empty values, non-numeric values, zero, negative values, and unreasonably large values that could cause resource problems.
- Display a clear validation message when the input is invalid.
- Initialize the open hash table using the selected capacity.

### Closed Hashing
- Add an independent option to configure the initial capacity of `HashSetClosed`.
- Use the same input validation and error-handling principles as Open Hashing.
- Preserve the existing closed-hashing collision-resolution algorithm and automatic rehashing behavior.

### User Interface
Integrate these options naturally into the existing interface.

The intended workflow is:

1. The user opens the application.
2. The user selects Open Hashing or Closed Hashing.
3. The application asks the user to choose the initial table size.
4. The user confirms the size.
5. The application initializes the selected hash table and opens its workspace.
6. The user can insert, search for, and remove values while watching the existing visualization and animation.

Display the current capacity clearly in the workspace. If the table is resized automatically through rehashing, update the displayed capacity.

**Do not force both hash tables to use the same size.** Each must retain its own independently selected capacity.

If the existing interface already has a suitable configuration screen, extend it instead of adding unnecessary screens or menus.

## 2. Preserve the Existing Hashing Rules

### Open Hashing — Separate Chaining
- Continue using the existing `LinkedList` class for collision chains.
- The chosen capacity determines the number of buckets.
- Calculate bucket indices using the existing hash function and the current table capacity.
- Multiple values that hash to the same bucket must remain connected through the linked list.
- Do not replace the custom linked list with `std::list` or another container.

### Closed Hashing — Linear Probing
- The chosen capacity determines the initial number of slots.
- Preserve linear probing and the existing collision-resolution behavior.
- Preserve automatic rehashing when the existing load-factor threshold is reached, approximately 0.5.
- Preserve the existing requirement for prime table capacities if that is how the current implementation is designed.
- If the user enters a non-prime capacity and the implementation requires prime capacities, either normalize it to the next supported prime or explain the adjustment clearly before initialization.
- Ensure rehashing uses the correct new capacity and reinserts existing values correctly.

Do not silently change the established hashing algorithms to implement this feature.

## 3. Create a Separate Text File for Each Hash Table

Each hashing method must have its own persistent, human-readable text file representing its current table.

Use these default filenames unless the existing project already has an established output directory:

- `output/open_hash_table.txt`
- `output/closed_hash_table.txt`

Create the `output` directory if it does not exist.

The two files must be independent. Updating one table must not overwrite the contents of the other table's file.

### Required File Format

The text files must look like formatted tables, not comma-separated lists or unstructured debug logs.

For Open Hashing, use a layout similar to this:

```text
OPEN HASHING TABLE
==================
Capacity: 11
Collision Method: Separate Chaining

+-------+-------------------------+
| Index | Data                    |
+-------+-------------------------+
| 0     | EMPTY                   |
| 1     | 15 -> 26 -> 37          |
| 2     | 8                       |
| 3     | EMPTY                   |
| 4     | 48 -> 59                |
| 5     | EMPTY                   |
| 6     | 17                      |
| 7     | EMPTY                   |
| 8     | 30                      |
| 9     | EMPTY                   |
| 10    | 10 -> 21                |
+-------+-------------------------+

Total Values: 8
```

The values above are examples only. The actual file must reflect the current table contents.

For Closed Hashing, use a layout similar to this:

```text
CLOSED HASHING TABLE
====================
Capacity: 11
Collision Method: Linear Probing

+-------+-------------------------+
| Index | Data                    |
+-------+-------------------------+
| 0     | 21                      |
| 1     | EMPTY                   |
| 2     | 13                      |
| 3     | EMPTY                   |
| 4     | 48                      |
| 5     | EMPTY                   |
| 6     | EMPTY                   |
| 7     | 18                      |
| 8     | EMPTY                   |
| 9     | EMPTY                   |
| 10    | 10                      |
+-------+-------------------------+

Total Values: 4
Load Factor: 0.36
```

The examples illustrate the formatting only. Do not hardcode these indices, values, capacities, or totals. Generate every row from the actual data structure.

### Formatting Requirements
- Use consistent column widths and alignment.
- Include every valid index from `0` to `capacity - 1`.
- Display `EMPTY` for an unused bucket or slot.
- For Open Hashing, show the values in each bucket's linked list in their actual chain order, separated by ` -> `.
- For Closed Hashing, display the value occupying each slot.
- Include the table's current capacity and collision-resolution method.
- Include the number of stored values.
- For Closed Hashing, include the current load factor.
- Ensure longer chains or larger indices do not break the table's formatting.
- Write readable text with appropriate line endings.
- Do not use placeholder rows or fabricated data.

## 4. Keep the Text Files Synchronized with the Actual Tables

The files must represent the **current state** of their respective data structures, not merely the last completed insertion.

Update the relevant file after every successful state-changing operation, including:

- Table initialization.
- Successful insertion.
- Successful removal.
- Clearing the table.
- Automatic rehashing.
- Any other operation that changes the table's contents or capacity.

Searching for a value should not change the file unless the existing implementation explicitly changes table state during searching.

If an operation fails without changing the table, do not write an inaccurate table representation.

### Initial State
Immediately after the user chooses a table size and the table is initialized, generate its text file, including all indices and `EMPTY` entries.

### Rehashing
When Closed Hashing automatically rehashes:
1. Complete the rehash using the existing algorithm.
2. Update the current capacity and load factor.
3. Write the new complete table to `closed_hash_table.txt`.
4. Ensure the file contains the new indices and actual positions of all stored values.

Do not leave the file showing the old table capacity after rehashing.

### Reliability
- Use safe file-handling practices and check whether the file can be opened and written.
- Handle file-writing errors without crashing the application.
- Display an understandable warning if a file cannot be updated.
- Never clear or truncate the other hashing method's file when updating the current one.
- Avoid writing the file repeatedly for every animation frame. Write when the actual table state changes or when a complete operation reaches a consistent state.
- Make sure the file-writing operation does not block the animation or freeze the UI unnecessarily.

## 5. Implement a Clean Design

Inspect the existing architecture and reuse its current classes and interfaces wherever practical.

If appropriate, add a dedicated helper such as `HashTableFileExporter` to handle text formatting and file output.

The exporter should:
- Accept or access the actual hash table state through a clean interface.
- Generate the formatted text dynamically.
- Handle Open Hashing and Closed Hashing appropriately.
- Keep file-writing logic separate from the hashing algorithms and window-rendering code.
- Avoid duplicating hash-table logic inside the exporter.

If the table classes do not expose enough information for accurate export, add minimal, safe read-only accessors or an appropriate export interface. Do not expose internal pointers unnecessarily or make the data structures publicly mutable.

Keep the existing `LinkedList`, `HashSetOpen`, and `HashSetClosed` implementations intact wherever possible.

## 6. Keep the Visualizer and File Output Consistent

The on-screen visualization and text file must describe the same table state.

For example, if the user selects an initial capacity of 7:
- The workspace must show the correct capacity.
- Open Hashing must show seven buckets, indexed from 0 through 6.
- Closed Hashing must show the supported capacity chosen by the application, clearly indicating any adjustment if necessary.
- The corresponding text file must contain exactly the same number of table rows as the actual capacity.
- Insertion animations must use the actual selected capacity and correct hash calculations.

Do not make the animation display one table size while the underlying data structure or text file uses another.

The text file is a representation of the real data structure, not a separate simulated table.

## 7. Testing Requirements

Add tests for the new functionality and run the existing regression tests.

At minimum, verify the following:

**Configurable capacities**
- Initialize Open Hashing with a capacity of 7.
- Initialize Closed Hashing with a supported capacity of 11.
- Verify that each table has the expected number of buckets or slots.
- Verify that the two methods can use different capacities.
- Test invalid, empty, negative, non-numeric, zero, and excessively large input.

**File generation**
- Confirm that both text files are created when their respective tables are initialized.
- Verify that every valid index is written exactly once.
- Verify that empty buckets and slots display `EMPTY`.
- Verify that linked-list chains are formatted correctly.
- Verify that capacity, stored-value count, and closed-hashing load factor are correct.

**File updates**
- Insert values and confirm the corresponding file changes.
- Remove values and confirm the corresponding file changes.
- Clear the table and confirm the file reflects the empty table.
- Trigger closed-hashing rehashing and confirm the file reflects the new capacity and correct values.
- Confirm that modifying Open Hashing does not alter the Closed Hashing file, and vice versa.

**Regression and stability**
- Verify that existing insertion, search, removal, collision handling, and animations still work.
- Verify that repeated operations do not produce stale or inconsistent files.
- Verify that file-writing failures do not crash the application.
- Verify that the application can switch between hashing workspaces without mixing their state or output files.

Use automated tests for formatting and data correctness where possible. Perform a manual GUI check for the capacity-selection workflow and visualization synchronization.

Do not claim that a test passed unless it was actually executed.

## 8. Completion Report

After implementing the changes, report:
1. Which files were added or modified.
2. How the user selects each hash table's initial capacity.
3. Where both output files are stored.
4. When each file is refreshed.
5. How invalid capacities are handled.
6. How rehashing affects the Closed Hashing file.
7. Which tests were executed and their actual results.
8. Any remaining limitations or unresolved issues.

**Priority:** Correctness of the real data structures first, independent table capacities second, accurate text-file exports third, and visual polish fourth. Preserve the current application's architecture and existing functionality.