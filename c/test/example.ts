// TypeScript example file
interface User {
  id: number;
  name: string;
  email: string;
  age?: number;
}

type UserRole = "admin" | "user" | "guest";

interface Greeter {
  greet(name: string): string;
}

const MAX_USERS: number = 100;
const MIN_AGE: number = 18;

let defaultGreeting: string = "Hello";

function createUser(id: number, name: string, email: string): User {
  return {
    id: id,
    name: name,
    email: email,
  };
}

function greetUser(user: User): string {
  return `${defaultGreeting}, ${user.name}!`;
}

function formatEmail(email: string): string {
  return email.toLowerCase();
}

function validateAge(age: number): boolean {
  return age >= MIN_AGE;
}

class UserManager implements Greeter {
  private users: User[] = [];

  constructor(private maxUsers: number = MAX_USERS) {}

  addUser(user: User): void {
    if (this.users.length < this.maxUsers) {
      this.users.push(user);
    }
  }

  greet(name: string): string {
    return `Hello, ${name}!`;
  }

  getUser(id: number): User | undefined {
    return this.users.find((u) => u.id === id);
  }
}

// Generic function
function identity<T>(arg: T): T {
  return arg;
}

// Arrow function with types
const add = (a: number, b: number): number => a + b;

// Union types
function process(value: string | number): void {
  console.log(value);
}

export { User, UserRole, createUser, UserManager };
