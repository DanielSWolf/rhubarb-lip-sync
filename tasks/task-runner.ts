// Simple task runner for Node.js

import { kebabCase } from 'change-case';

/** A build task. */
export interface Task<TResult = unknown> {
  /** The fully qualified kebab-case name of the task. */
  readonly name: string;

  /** A human-readable description of the task's purpose. */
  readonly description: string;

  /**
   * The tasks and task groups that need to have run before this task.
   * These may be run in any order, including in parallel.
   */
  readonly dependencies: Array<Task | TaskGroup>;

  /** The function performing the task's purpose. */
  readonly fn: TaskFn<TResult>;

  /** The result of the task. Getter throws if accessed before the task has finished. */
  readonly result: TResult;

  /** @internal */
  readonly _setName: (name: string) => void;

  /** @internal */
  readonly _setResult: (result: TResult) => void;
}

/** A group of build tasks that can be run in parallel. */
export type TaskGroup<TChildren extends Record<string, Task | TaskGroup> = {}> = {
  /** The fully qualified kebab-case name of the task group. */
  readonly name: string;

  /** A human-readable description of the task group's purpose. */
  readonly description: string;

  /**
   * The child tasks and task groups.
   * For convenience, these are also available as properties of this task group object.
   */
  readonly children: TChildren;

  /** @internal */
  readonly _setName: (name: string) => void;
} & TChildren;

/** The function performing a task's purpose. */
export type TaskFn<TResult> = (args: TaskArgs) => Promise<TResult>;

export interface TaskArgs {
  /** Sets the current status of the task. */
  setStatus: TaskStatusSetter;

  /**
   * The stream to use for any output, rather than using `console.log`. May or may not be aggregated
   * before being printed.
   */
  out: WritableStream;

  /** Skips this task by throwing a {@link SkipError}. */
  skip: () => never;
}

/** Indicates that a task has been skipped because its purpose has already been achieved. */
export class SkipError extends Error {
  constructor() {
    super('Task skipped because its purpose has already been achieved.');
    this.name = new.target.name;
  }
}

/**
 * Sets the visible status of the task.
 * @param message A human-readable description of the task's current status. Must be short enough to
 * fit on one terminal line, together with the progress, if any.
 * @param progress Additional progress information, such as '70%' or '4.5 MB of 12.3 MB'.
 */
export type TaskStatusSetter = (message: string, progress?: string) => void;

/** Null-like value for situations where `null` and `undefined` might be valid values. */
const none = Symbol('none');

/** Creates a {@link Task}. */
export function createTask<TResult>(args: CreateTaskArgs, fn: TaskFn<TResult>): Task<TResult> {
  let name: string | null = null;
  let taskResult: TResult | typeof none = none;

  return {
    get name() {
      if (name === null) throw new Error('Task name not set yet.');
      return name;
    },
    description: args.description,
    dependencies: args.dependencies ?? [],
    fn,
    get result() {
      if (taskResult === none) throw new Error('Task result not available yet.');
      return taskResult;
    },

    _setName: _name => {
      if (name !== null) throw new Error('Task name already set.');
      name = _name;
    },
    _setResult: _taskResult => {
      if (taskResult !== none) throw new Error('Task result already set.');
      taskResult = _taskResult;
    },
  };
}

export interface CreateTaskArgs {
  /** A human-readable description of the task's purpose. */
  description: string;

  /**
   * The tasks and task groups that need to have run before this task.
   * These may be run in any order, including in parallel.
   */
  dependencies?: Array<Task | TaskGroup>;
}

/** Creates a {@link TaskGroup}. */
export function createTaskGroup<TChildren extends Record<string, Task | TaskGroup> = {}>(
  args: CreateTaskGroupArgs,
  children: TChildren
): TaskGroup<TChildren> {
  let name: string | null = null;

  return {
    get name() {
      if (name === null) throw new Error('Task group name not set yet.');
      return name;
    },
    description: args.description,
    children,
    ...children,

    _setName: _name => {
      if (name !== null) throw new Error('Task group name already set.');
      name = _name;
    },
  };
}

export interface CreateTaskGroupArgs {
  /** A human-readable description of the task group's purpose. */
  description: string;
}

/** Runs the task(s) indicated by the command line, then exits with an appropriate exit code. */
export function taskRunnerMain(allTasks: TaskGroup): Promise<void> {
  setNames(allTasks);

  throw new Error('Not implemented yet.');
}

/** Sets the `name` property on this task groups and all its descendents. */
function setNames(group: TaskGroup, prefix: string = '') {
  const childEntries = Object.entries(group.children) as Array<[string, Task | TaskGroup]>;
  for (const [rawName, child] of childEntries) {
    const name = prefix + kebabCase(rawName);
    if ('_setName' in child) {
      child._setName(name);
    } else {
      setNames(child, name + ':');
    }
  }
}
