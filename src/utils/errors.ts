export class CompatibilityError extends Error {
    constructor() {
        super('This platform/manager is not compatible with this package');
    }
}

/* eslint-disable @typescript-eslint/no-unused-vars */
export function throwCompatibilityError(...args: any[]): never {
    throw new CompatibilityError();
}
/* eslint-enable @typescript-eslint/no-unused-vars */