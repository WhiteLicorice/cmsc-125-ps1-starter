<!--no-pdf-->
# Your Own Case

This folder holds the one boundary or failure case your pair writes for the
rubric's implementation row. The published corpus lives in `cases/`. Keep it unchanged.

The three files named `example` form one case. Replace them with your own case.
A case you write earns marks. Replace the example when you have a case of your
own.

## One Case, Three Files

A case is a `.work` file with two neighbours.

- `name.work` holds the workload, one worker on a line.
- `name.args` holds the command line, such as `--policy rr --quantum 4`.
- `name.expect` holds the expected exit code and the expected output.

The manual's "The Workload File" and "The Run Contract" sections state every
rule. The runner reads the same three files for the published corpus, so a case
here is written the same way.

An `.expect` file holds one directive on a line.

| Directive | Meaning |
|---|---|
| `exit <code>` | the exit code the run must return |
| `stderr <text>` | a substring stderr must hold, repeatable |
| `schedule <w>@<tick>` | the dispatch order and the tick of each dispatch |
| `interrupt SIGINT` | send `SIGINT` to the controller after a short delay |
| `path <relative>` | pass this workload path instead of the case's `.work` file |
| `partial` | accept a partial report, as `SIGINT` and a fault produce |

## Run Your Case

```bash
make
./check.sh student-cases
```

`./check.sh` with no argument runs the published corpus in `cases/`. With
`student-cases` it runs the cases in this folder instead.

## Show It Detects a Defect

The rubric asks for the failing record, not only a green case. Break one
mechanism in `src/`, run `./check.sh student-cases`, and keep the output that
names your case. Revert the break. Paste the record into `ANALYSIS.md`.

A case that stays green with the defect present proves nothing.
