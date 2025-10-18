# 🎟️ Universal Reservation System in C

![Language](https://img.shields.io/badge/language-C-blue.svg)
![Status](https://img.shields.io/badge/build-passing-brightgreen.svg)
![License](https://img.shields.io/badge/license-MIT-yellow.svg)
![Platform](https://img.shields.io/badge/platform-Terminal-lightgrey.svg)

> A **menu-driven reservation management system** built in pure **C language** — featuring booking, waitlisting, cancellation, modification, search, undo, and slot management functionalities.

---

## ✨ Features

| 🚀 Functionality | 💡 Description |
|------------------|----------------|
| 🆕 **New Reservation** | Add new bookings with auto-generated Reservation ID and slot allocation. |
| ⏳ **Waitlist Handling** | Automatically adds users to a queue when all slots are booked. |
| ❌ **Cancel Reservation** | Frees a slot and auto-promotes the next waitlisted user. |
| 🧾 **Modify Details** | Edit customer info such as name, age, and contact. |
| 🔍 **Search Reservation** | Find reservation details using the Reservation ID. |
| 🔄 **Undo Last Booking** | Reverts the most recent booking instantly. |
| 🪑 **Slot Map** | Displays slot status (booked/available). |
| 📊 **Check Availability** | Shows total, booked, and available slots. |
| ⚙️ **Change Total Slots** | Dynamically increase or decrease total available slots. |

---

## 🧠 Data Structures Used

- **Linked List** → For managing confirmed and waitlisted customers.  
- **Stack** → For implementing the Undo operation.  
- **Dynamic Memory Allocation** → For flexible record management.  

---

## 🖥️ How It Works

1. System starts with a fixed number of slots (default: **5**).  
2. Each new reservation is assigned a slot and unique ID.  
3. When all slots are full, new customers are placed on a **waitlist**.  
4. On cancellation, the next waitlisted customer is **auto-promoted**.  
5. Undo can reverse the last confirmed booking.

---

## ⚙️ Compilation & Execution

### 🛠️ Compile
```bash
gcc reservation_system.c -o reservation_system
```
## ▶️ Run 
```bash
./reservation_system
```
## 🧾 Menu Options
- Option	Description.
1.	New Reservation.
2.	Cancel Reservation.
3.	Modify Customer Details.
4.	Search Reservation.
5.	Show Confirmed List.
6.	Show Waitlist.
7.	Show Slot Map.
8.	Check Availability.
9.	Change Total Slots.
10.	Undo Last Booking.
11.	Exit.
## 🧩 Example Output
```yaml
UNIVERSAL RESERVATION SYSTEM

1. New Reservation
2. Cancel Reservation
3. Modify Customer Details
...
Enter choice: 1
Enter Name: Alice
Enter Age: 24
Enter Contact: 9876543210
Reservation confirmed for Alice (Slot 1)
Your Reservation ID is: 1000
```

## 📂 Project Structure
```bash
├── reservation_system.c     # Main program file
├── README.md                # Project documentation
```

## 🚀 Future Enhancements

- 💾 Add file storage for data persistence.

- 🔐 User & Admin authentication system.

- 🖥️ GUI (Graphical User Interface) version.

- ☁️ Cloud database integration.

## 👨‍💻 Author

Piyush Gairola
📧 [piyushgairola32@gmail.com]


## ⭐ If you found this project helpful, please consider giving it a star on GitHub!
