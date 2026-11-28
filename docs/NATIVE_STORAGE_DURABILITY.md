# Native profile and library write durability

The Library and active-profile stores write to a unique sibling temporary file created with mkstemp, rather than opening a predictable .tmp path.

The write sequence is:

1. Create a fresh exclusive temporary file in the same directory as the destination.
2. Write the complete serialized record.
3. Flush stdio and call fsync on the temporary file descriptor.
4. Close the file.
5. Atomically rename the temporary file over the destination.
6. Remove the temporary file on any pre-rename error.

This prevents a stale or attacker-planted .tmp symlink from redirecting writes into an unrelated file. The temporary file is created with the restrictive permissions supplied by mkstemp.

Host regression tests create the old predictable .tmp symlink for both stores, perform a save, and verify that the symlink target remains unchanged. This is a host filesystem test; Vita storage behavior and power-loss durability still require device validation.
