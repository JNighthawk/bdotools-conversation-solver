# bdotools-conversation-solver
Solver application for the conversation mini-game in Black Desert Online

Given parameters presented by the conversation mini-game, this will attempt to find the solution with the highest chance of success. It has two main modes of operation: manual input, and automated via command line.

Command line interface:
- SolveAll, SolveAllFast, SolveAllMin, SolveAllMinFast, SolveAllTarget, SolveAllTargetFast

This uses libpq for database interactions, with a PostgresSQL database. Data read from the database:
- Knowledge
- Knowledge categories
- Constellations
- Target NPCs

Results are stored in the database, identified by:
- Target NPC
- Target interest level
- Target favor level
- Conversation goal type
- Conversation goal quantity
- App version

TODO:
- Upload current database
- Make this readme better
