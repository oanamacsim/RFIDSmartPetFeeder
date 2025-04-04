import SwiftUI

struct AddFeederView: View {
    @Binding var feeders: [Feeder] // Binding to feeders list
    @Environment(\.presentationMode) var presentationMode
    
    @State private var id = ""
    @State private var password = ""
    @State private var name = ""
    @State private var isLoading = false
    @State private var errorMessage: String?
    @State private var selectedFeeder: Feeder? = nil
    
    var updateFeeder: (Feeder, Bool) -> Void  // Closure to update feeder
    
    var body: some View {
        VStack(spacing: 20) {
            Text("Adaugă un Feeder Nou")
                .font(.largeTitle)
                .fontWeight(.bold)
                .padding(.top)
            
            Form {
                Section(header: Text("Detalii Feeder")) {
                    TextField("ID Feeder", text: $id)
                        .onChange(of: id) { newValue in id = newValue.lowercased() }
                        .textInputAutocapitalization(.never) // Disable automatic capitalization
                        .autocorrectionDisabled(true)       // Optional: Disable autocorrection
                        .textFieldStyle(RoundedBorderTextFieldStyle())
                    
                    SecureField("Parolă", text: $password)
                        .textFieldStyle(RoundedBorderTextFieldStyle())
                    
                    TextField("Nume Feeder", text: $name)
                        .textFieldStyle(RoundedBorderTextFieldStyle())
                }
            }
            .padding(.horizontal)
            
            if let error = errorMessage {
                Text(error)
                    .foregroundColor(.red)
                    .padding()
            }
            
            Button(action: addFeeder) {
                HStack {
                    if isLoading {
                        ProgressView()
                    } else {
                        Image(systemName: "plus.circle.fill")
                            .font(.title)
                        Text("Conectează și Salvează")
                            .font(.headline)
                    }
                }
                .padding()
                .frame(maxWidth: .infinity)
                .background(Color.green)
                .foregroundColor(.white)
                .cornerRadius(12)
            }
            .padding()
            .disabled(isLoading)
            
            Spacer()
            
            NavigationLink(destination: selectedFeeder.map { FeederView(feeder: $0, updateFeeder: updateFeeder) }) {
                EmptyView()
            }
        }
        .background(Color(UIColor.systemGroupedBackground))
    }
    
    func addFeeder() {
        isLoading = true
        errorMessage = nil
        
        // Check for duplicates
        if feeders.contains(where: { $0.id.lowercased() == id.lowercased() }) {
            errorMessage = "Feeder-ul există deja."
            isLoading = false
            return
        }
        
        let urlString = "https://dev.bull-software.com/get_feeder.php?ID=\(id)&Password=\(password)&add_if_not_found=true&Name=\(name)"
        guard let url = URL(string: urlString) else {
            errorMessage = "URL invalid."
            isLoading = false
            return
        }
        
        URLSession.shared.dataTask(with: url) { data, response, error in
            DispatchQueue.main.async {
                isLoading = false
                
                if let error = error {
                    errorMessage = "Eroare de rețea: \(error.localizedDescription)"
                    return
                }
                
                guard let data = data, let httpResponse = response as? HTTPURLResponse else {
                    errorMessage = "Răspuns invalid de la server."
                    return
                }
                
                if httpResponse.statusCode != 200 {
                    do {
                        let errorResponse = try JSONDecoder().decode([String: String].self, from: data)
                        errorMessage = errorResponse["error"] ?? "Eroare necunoscută."
                    } catch {
                        errorMessage = "Eroare necunoscută."
                    }
                    return
                }
                
                // Parse valid response
                do {
                    let feederData = try JSONDecoder().decode(FeederResponse.self, from: data)
                    
                    let newFeeder = Feeder(
                        id: feederData.ID,
                        password: password,
                        name: name,
                        trapMode: feederData.TrapMode,
                        feedConfig: feederData.FeedFoodConfiguration,
                        foodStorageQuantity: feederData.FoodStorageQuantity,
                        lastFoodStorageQuantityUpdateTime: feederData.LastFoodStorageQuantityUpdateTime,
                        foodCurrentWeight: feederData.FoodCurrentWeight,
                        lastFoodCurrentWeightUpdateTime: feederData.LastFoodCurrentWeightUpdateTime
                    )
                    
                    feeders.append(newFeeder)
                    
                    // Save to cache (UserDefaults)
                    if let encoded = try? JSONEncoder().encode(feeders) {
                        UserDefaults.standard.set(encoded, forKey: "savedFeeders")
                    }
                    
                    presentationMode.wrappedValue.dismiss()
                    
                    selectedFeeder = newFeeder
                } catch {
                    errorMessage = "Eroare la procesarea datelor."
                }
            }
        }.resume()
    }
}

struct FeederResponse: Codable {
    let ID: String
    let Name: String
    let TrapMode: String
    let FeedFoodConfiguration: [String: Int]
    let FoodStorageQuantity: Double
    let LastFoodStorageQuantityUpdateTime: Int
    let FoodCurrentWeight : Double
    let LastFoodCurrentWeightUpdateTime: Int
}
