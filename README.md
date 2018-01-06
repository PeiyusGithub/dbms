# Database Management System
This project implemented the following parts in database management system : **Record-Based File Manager, Relation Manager, Index Manager, and Query Engine**. Which are basic and important in the DBMS Architecture.
![DBMS Architecture](https://i.loli.net/2018/01/06/5a5008c380bbb.png)

## Record-Based File Manager
I implement a simple paged file(PF) manager, it builds up the basic file system required for the rest projects. The PF component provides facilities for higher-level client components to perform file I/O in terms of pages. In the PF component, methods are provided to create, destroy, open, and close paged files, to read and write a specific page of a given file, and to add pages to a given file. 
Based on the PF manager, I implemented the record-based file manager.

## Relation Manager
The Relation Manager class is responsible for managing the database tables. It handles the creation and deletion of tables. It also handles the basic operations performed on top of a table (e.g., insert and delete tuples).

## Index Manager
The IX component provides classes and methods for managing persistent indexes over unordered data records stored in files. Each data file may have any number of (single-attribute) indexes associated with it. The indexes ultimately will be used to speed up processing of relational selections, joins, condition-based update, and delete operations. In this part I implement a basic B+ tree that has support for range predicates.

## Query Engine
The QE component provides classes and methods for answering SQL queries. In this part, I implemented the Block Nested-Loop Join, Index Nested-Loop Join and Grace Hash Join interface.