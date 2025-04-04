import SwiftUI

struct NavigationFeederView: View {
    @State private var feeders: [Feeder] = []
    @State private var feederToDelete: Feeder? = nil
    @State private var showAlert = false
    
    var body: some View {
        NavigationView {
            VStack {
                if feeders.isEmpty {
                    Text("No Feeders available")
                        .font(.title)
                        .foregroundColor(.gray)
                } else {
                    List {
                        ForEach(feeders) { feeder in
                            HStack {
                                NavigationLink(destination: FeederView(feeder: feeder, updateFeeder: updateFeeder)) {
                                    Text(feeder.name)
                                        .font(.headline)
                                }
                                Spacer()
                                Button(action: {
                                    feederToDelete = feeder
                                    showAlert = true
                                }) {
                                    Image(systemName: "trash")
                                        .foregroundColor(.red)
                                }
                                .buttonStyle(BorderlessButtonStyle())
                            }
                        }
                    }
                }
                
            }
            .navigationTitle("Feedere")
            .toolbar {
                NavigationLink(destination: AddFeederView(feeders: $feeders, updateFeeder: updateFeeder)) {
                    Image(systemName: "plus.circle.fill")
                }
            }
            .onAppear {
                loadFeeders()
            }
            .alert("Are you sure you want to delete this feeder?", isPresented: $showAlert) {
                Button("Cancel", role: .cancel) {}
                Button("Delete", role: .destructive) {
                    if let feederToDelete = feederToDelete {
                        removeFeeder(feederToDelete)
                    }
                }
            }
        }
    }
    
    private func loadFeeders() {
        if let savedData = UserDefaults.standard.data(forKey: "savedFeeders") {
            if let decodedFeeders = try? JSONDecoder().decode([Feeder].self, from: savedData) {
                feeders = decodedFeeders
            }
        }
    }
    
    private func removeFeeder(_ feeder: Feeder) {
        feeders.removeAll { $0.id == feeder.id }
        saveFeeders()
    }
    
    private func saveFeeders() {
        if let encoded = try? JSONEncoder().encode(feeders) {
            UserDefaults.standard.set(encoded, forKey: "savedFeeders")
        }
    }
    
    func updateFeeder(feeder : Feeder, updateDB: Bool)
    {
        if let index = feeders.firstIndex(where: { $0.id == feeder.id })
        {
            // update feeder
            feeders[index] = feeder
            
            // save the feeders on disk memory
            saveFeeders()
            
            // Consider doing an http request here to also update the entries from the DB
            if (updateDB)
            {
                updateFeederInDB(feeder: feeder) { success, message in
                    if success {
                        print("Success: \(message ?? "Feeder updated.")")
                    } else {
                        print("Failed: \(message ?? "An unknown error occurred.")")
                    }
                }
            }
        }
    }
    
    func updateFeederInDB(feeder: Feeder, completion: @escaping (Bool, String?) -> Void)
    {
        // Define the API URL
        guard let url = URL(string: "https://dev.bull-software.com/update_feeder.php") else {
            completion(false, "Invalid URL.")
            return
        }
        
        // Create the JSON payload
        let payload: [String: Any] = [
            "ID": feeder.id,
            "Password": feeder.password,
            "TrapMode": feeder.trapMode,
            "FeedFoodConfiguration": feeder.feedConfig,
            "FoodStorageQuantity": feeder.foodStorageQuantity,
            "LastFoodStorageQuantityUpdateTime": feeder.lastFoodStorageQuantityUpdateTime,
            "FoodCurrentWeight": feeder.foodCurrentWeight,
            "LastFoodCurrentWeightUpdateTime": feeder.lastFoodCurrentWeightUpdateTime
        ]
        
        // Convert the payload to JSON
        guard let jsonData = try? JSONSerialization.data(withJSONObject: payload, options: []) else {
            completion(false, "Failed to encode payload.")
            return
        }
        
        // Create the PUT request
        var request = URLRequest(url: url)
        request.httpMethod = "PUT"
        request.setValue("application/json", forHTTPHeaderField: "Content-Type")
        request.httpBody = jsonData
        
        // Perform the network request
        URLSession.shared.dataTask(with: request) { data, response, error in
            if let error = error {
                completion(false, "Request failed: \(error.localizedDescription)")
                return
            }
            
            guard let httpResponse = response as? HTTPURLResponse else {
                completion(false, "Invalid response.")
                return
            }
            
            // Handle the server's response
            switch httpResponse.statusCode {
            case 200...299:
                // Parse the response data
                if let data = data,
                   let json = try? JSONSerialization.jsonObject(with: data, options: []) as? [String: Any],
                   let message = json["message"] as? String {
                    completion(true, message)
                } else {
                    completion(true, "Feeder updated successfully.")
                }
            case 400...499:
                if let data = data,
                   let json = try? JSONSerialization.jsonObject(with: data, options: []) as? [String: Any],
                   let errorMessage = json["error"] as? String {
                    completion(false, errorMessage)
                } else {
                    completion(false, "Client error: \(httpResponse.statusCode).")
                }
            case 500...599:
                completion(false, "Server error: \(httpResponse.statusCode).")
            default:
                completion(false, "Unexpected response code: \(httpResponse.statusCode).")
            }
        }.resume()
    }
}

