import { ExecException, exec } from 'child_process';

export const execCommand = (cmd: string, args: string[]): Promise<string> =>
  new Promise((resolve, reject) => {
    exec(
      `${cmd} ${
        args
          .map(arg => typeof arg === 'string' ? `"${arg}"` : arg)
          .join(' ')
      }`,
      (err: ExecException, stdout: string, stderr: string) => {
        if (err || stderr) {
          reject(stderr);
        } else {
          resolve(stdout);
        }
      },
    );
  });