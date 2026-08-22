
Data Model:
    - Character
    - ID
    - Tombstone


Applying Operations
Insert Alg:
    - Insert after element
Delete Alg:
    - Mark deleted
Invarients:
1. what does it read from the operation: What type of operation is it
2. what does it need to access in Document: The vector elements
3. what invariants must still be true afterwards: unique ids, all neighbors exist, tombstones still exist, the doc can still be rendered


Ordering Rule:
    - Lexicographic ID comparison

Invarients:

IDs are unique.
Deleted elements are never removed
Rendering skips tombstones
Applying the same operatttion twice does not change the document (idempotent)



Throughline of CRDT:
Cursor position
    ↓
Left neighbor ID
Right neighbor ID
    ↓
ID Generator
    ↓
New ElementID
    ↓
InsertOperation
    ↓
Document



Throughput of server:

EditorSession A
      │
      │ take_outgoing_operations()
      ▼
NetworkClient A
      │
      │ serialize + frame + TCP
      ▼
Server
      │
      │ receive + route
      ▼
ClientConnection B
      │
      │ frame + TCP
      ▼
NetworkClient B
      │
      │ deserialize
      ▼
EditorSession B
      │
      │ receive_message()
      ▼
flush_incoming()
      │
      ▼
Document B



                     Server
                       │
             ┌─────────┴─────────┐
             │                   │
        OperationLog     PersistentOperationLog
             │                   │
        synchronization          │
             │                   ↓
             │              operations.log
             │
             ↓
         new clients



START
  │
  ├── Load persistent operations
  │
  ├── Populate OperationLog
  │
  └── Begin accepting clients
          │
          ↓
       Client connects
          │
          ↓
      Send history
          │
          ↓
        SYNC
          │
          ↓
        LIVE
          │
          ↓
     New operation
          │
          ├── OperationLog
          ├── PersistentOperationLog
          └── Route to clients